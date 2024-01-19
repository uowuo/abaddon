#pragma once
#include <curl/curl.h>
#include <functional>
#include <string>
#include <filesystem>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <future>
#include <mutex>
#include <queue>
#include "http.hpp"

class FileCacheWorkerThread {
public:
    using callback_type = sigc::slot<void(std::string path)>;

    FileCacheWorkerThread();
    ~FileCacheWorkerThread();

    void set_file_path(const std::filesystem::path &path);

    void add_image(const std::string &string, callback_type callback);

    void stop();

private:
    void loop();

    bool m_stop = false;
    std::thread m_thread;

    struct QueueEntry {
        std::string URL;
        callback_type Callback;
    };

    std::condition_variable m_cv;

    mutable std::mutex m_queue_mutex;
    std::queue<QueueEntry> m_queue;

    std::unordered_map<CURL *, FILE *> m_curl_file_handles;
    std::unordered_map<CURL *, std::string> m_handle_urls;
    std::unordered_map<std::string, std::filesystem::path> m_paths;
    std::unordered_map<std::string, callback_type> m_callbacks;

    int m_running_handles = 0;

    std::unordered_set<CURL *> m_handles;
    CURLM *m_multi_handle;

    std::filesystem::path m_data_path;
};

class Cache {
public:
    Cache();
    ~Cache();

    using callback_type = std::function<void(std::string)>;
    void GetFileFromURL(const std::string &url, const callback_type &cb);
    std::string GetPathIfCached(const std::string &url);
    void ClearCache();

private:
    void CleanupFutures();
    static void RespondFromPath(const std::filesystem::path &path, const callback_type &cb);
    void OnResponse(const std::string &url);
    void OnFetchComplete(const std::string &url);

    std::unordered_map<std::string, std::vector<callback_type>> m_callbacks;
    std::vector<std::future<void>> m_futures;
    std::filesystem::path m_tmp_path;

    mutable std::mutex m_mutex;

    FileCacheWorkerThread m_worker;
};
