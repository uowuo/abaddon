#include "abaddon.hpp"
#include "filecache.hpp"

constexpr static const int MaxConcurrentCacheHTTP = 10;

Cache::Cache() {
    m_tmp_path = std::filesystem::temp_directory_path() / "abaddon-cache";
    std::filesystem::create_directories(m_tmp_path);
}

Cache::~Cache() {
    std::error_code err;
    if (!std::filesystem::remove_all(m_tmp_path, err))
        fprintf(stderr, "error removing tmp dir\n");
}

std::string Cache::SanitizeString(std::string str) {
    std::string ret;
    for (const char c : str) {
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z'))
            ret += c;
        else
            ret += '_';
    }
    return ret;
}

void Cache::RespondFromPath(std::filesystem::path path, callback_type cb) {
    cb(path.string());
}

void Cache::GetFileFromURL(std::string url, callback_type cb) {
    auto cache_path = m_tmp_path / SanitizeString(url);
    if (std::filesystem::exists(cache_path)) {
        m_futures.push_back(std::async(std::launch::async, [this, cache_path, cb]() { RespondFromPath(cache_path, cb); }));
        return;
    }

    // needs to be initialized like this or else ::Get() is called recursively
    if (!m_semaphore)
        m_semaphore = std::make_unique<Semaphore>(Abaddon::Get().GetSettings().GetSettingInt("http", "concurrent", MaxConcurrentCacheHTTP));

    if (m_callbacks.find(url) != m_callbacks.end()) {
        m_callbacks[url].push_back(cb);
    } else {
        m_callbacks[url].push_back(cb);
        auto future = std::async(std::launch::async, [this, url]() {
            m_semaphore->wait();
            const auto &r = cpr::Get(cpr::Url { url });
            m_semaphore->notify();
            OnResponse(r);
        });
        m_futures.push_back(std::move(future));
    }
}

std::string Cache::GetPathIfCached(std::string url) {
    auto cache_path = m_tmp_path / SanitizeString(url);
    if (std::filesystem::exists(cache_path)) {
        return cache_path.string();
    }

    return "";
}

// this just seems really yucky
void Cache::CleanupFutures() {
    for (auto it = m_futures.begin(); it != m_futures.end();) {
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            it = m_futures.erase(it);
        else
            it++;
    }
}

void Cache::OnResponse(const cpr::Response &r) {
    CleanupFutures(); // see above comment
    if (r.error || r.status_code > 300) return;

    std::vector<uint8_t> data(r.text.begin(), r.text.end());
    auto path = m_tmp_path / SanitizeString(r.url);
    FILE *fp = std::fopen(path.string().c_str(), "wb");
    if (fp == nullptr)
        return;
    std::fwrite(data.data(), 1, data.size(), fp);
    std::fclose(fp);

    for (const auto &cb : m_callbacks[r.url]) {
        cb(path.string());
    }
    m_callbacks.erase(r.url);
}
