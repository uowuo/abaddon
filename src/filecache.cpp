#include "filecache.hpp"

#include <utility>

#include "abaddon.hpp"
#include "MurmurHash3.h"

std::string GetCachedName(const std::string &str) {
    uint32_t out;
    MurmurHash3_x86_32(str.c_str(), static_cast<int>(str.size()), 0, &out);
    return std::to_string(out);
}

Cache::Cache() {
    m_tmp_path = std::filesystem::temp_directory_path() / "abaddon-cache";
    std::filesystem::create_directories(m_tmp_path);
    m_worker.set_file_path(m_tmp_path);
}

Cache::~Cache() {
    m_worker.stop();

    for (auto &future : m_futures) {
        if (future.valid()) {
            try { // dont care about stored exceptions
                future.get();
            } catch (...) {}
        }
    }

    std::error_code err;
    if (!std::filesystem::remove_all(m_tmp_path, err))
        fprintf(stderr, "error removing tmp dir\n");
}

void Cache::ClearCache() {
    for (const auto &path : std::filesystem::directory_iterator(m_tmp_path))
        std::filesystem::remove_all(path);
}

void Cache::RespondFromPath(const std::filesystem::path &path, const callback_type &cb) {
    cb(path.string());
}

void Cache::GetFileFromURL(const std::string &url, const callback_type &cb) {
    auto cache_path = m_tmp_path / GetCachedName(url);
    if (std::filesystem::exists(cache_path)) {
        m_mutex.lock();
        m_futures.push_back(std::async(std::launch::async, [cache_path, cb]() { RespondFromPath(cache_path, cb); }));
        m_mutex.unlock();
        return;
    }

    if (m_callbacks.find(url) != m_callbacks.end()) {
        m_callbacks[url].push_back(cb);
    } else {
        m_callbacks[url].push_back(cb);
        m_worker.add_image(url, [this, url](const std::string &path) {
            OnFetchComplete(url);
        });
    }
}

std::string Cache::GetPathIfCached(const std::string &url) {
    auto cache_path = m_tmp_path / GetCachedName(url);
    if (std::filesystem::exists(cache_path)) {
        return cache_path.string();
    }

    return "";
}

// this just seems really yucky
void Cache::CleanupFutures() {
    std::lock_guard<std::mutex> l(m_mutex);
    for (auto it = m_futures.begin(); it != m_futures.end();) {
        if (it->valid() && it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            it = m_futures.erase(it);
        else
            it++;
    }
}

void Cache::OnResponse(const std::string &url) {
    CleanupFutures(); // see above comment

    auto path = m_tmp_path / GetCachedName(url);

    m_mutex.lock();
    const auto key = static_cast<std::string>(url);
    auto callbacks = std::move(m_callbacks[key]);
    m_callbacks.erase(key);
    m_mutex.unlock();
    for (const auto &cb : callbacks)
        cb(path.string());
}

void Cache::OnFetchComplete(const std::string &url) {
    m_mutex.lock();
    m_futures.push_back(std::async(std::launch::async, [this, url] { OnResponse(url); }));
    m_mutex.unlock();
}

FileCacheWorkerThread::FileCacheWorkerThread() {
    m_multi_handle = curl_multi_init();
    m_thread = std::thread([this] { loop(); });
}

FileCacheWorkerThread::~FileCacheWorkerThread() {
    if (!m_stop) stop();
    for (const auto handle : m_handles)
        curl_easy_cleanup(handle);
    curl_multi_cleanup(m_multi_handle);
}

void FileCacheWorkerThread::set_file_path(const std::filesystem::path &path) {
    m_data_path = path;
}

void FileCacheWorkerThread::add_image(const std::string &string, callback_type callback) {
    m_queue_mutex.lock();
    m_queue.push({ string, std::move(callback) });
    m_cv.notify_one();
    m_queue_mutex.unlock();
}

void FileCacheWorkerThread::stop() {
    m_stop = true;
    if (m_thread.joinable()) {
        m_cv.notify_all();
        m_thread.join();
    }
}

void FileCacheWorkerThread::loop() {
    timeval timeout {};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while (!m_stop) {
        if (m_handles.empty()) {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            if (m_queue.empty())
                m_cv.wait(lock);
        }

        static const auto concurrency = static_cast<size_t>(Abaddon::Get().GetSettings().CacheHTTPConcurrency);
        if (m_handles.size() < concurrency) {
            std::optional<QueueEntry> entry;
            m_queue_mutex.lock();
            if (!m_queue.empty()) {
                entry = std::move(m_queue.front());
                m_queue.pop();
            }
            m_queue_mutex.unlock();

            if (entry.has_value()) {
                if (m_callbacks.find(entry->URL) != m_callbacks.end()) {
                    printf("url is being requested twice :(\n");
                    continue;
                }

                // add the ! and rename after so the image loader thing doesnt pick it up if its not done yet
                auto path = m_data_path / (GetCachedName(entry->URL) + "!");
                FILE *fp = std::fopen(path.string().c_str(), "wb");
                if (fp == nullptr) {
                    printf("couldn't open fp\n");
                    continue;
                }

                CURL *handle = curl_easy_init();
                m_handles.insert(handle);
                curl_easy_setopt(handle, CURLOPT_URL, entry->URL.c_str());
                curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);

                m_handle_urls[handle] = entry->URL;
                m_curl_file_handles[handle] = fp;
                m_callbacks[entry->URL] = entry->Callback;
                m_paths[entry->URL] = std::move(path);

                curl_multi_add_handle(m_multi_handle, handle);
            }
        }

        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;
        int maxfd = -1;
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);
        curl_multi_fdset(m_multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
        if (maxfd == -1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
        }

        curl_multi_perform(m_multi_handle, &m_running_handles);

        int num_msgs;
        while (auto msg = curl_multi_info_read(m_multi_handle, &num_msgs)) {
            if (msg->msg == CURLMSG_DONE) {
                auto url = m_handle_urls.at(msg->easy_handle);
                auto fp = m_curl_file_handles.find(msg->easy_handle);
                std::fclose(fp->second);

                m_handles.erase(msg->easy_handle);
                m_handle_urls.erase(msg->easy_handle);

                curl_multi_remove_handle(m_multi_handle, msg->easy_handle);
                curl_easy_cleanup(msg->easy_handle);

                auto path = m_paths.at(url).string();
                auto cb = m_callbacks.at(url);
                m_callbacks.erase(url);
                m_paths.erase(url);
                m_curl_file_handles.erase(fp);
                // chop off the !
                auto old = path;
                path.pop_back();
                std::filesystem::rename(old, path);
                cb(path);
            }
        }
    }
}
