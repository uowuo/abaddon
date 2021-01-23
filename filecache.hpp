#pragma once
#include <functional>
#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <future>
#include "util.hpp"
#include "http.hpp"

class Cache {
public:
    Cache();
    ~Cache();

    using callback_type = std::function<void(std::string)>;
    void GetFileFromURL(std::string url, callback_type cb);
    std::string GetPathIfCached(std::string url);
    void ClearCache();

private:
    std::string GetCachedName(std::string str);
    void CleanupFutures();
    void RespondFromPath(std::filesystem::path path, callback_type cb);
    void OnResponse(const http::response_type &r);

    std::unique_ptr<Semaphore> m_semaphore;

    std::unordered_map<std::string, std::vector<callback_type>> m_callbacks;
    std::vector<std::future<void>> m_futures;
    std::filesystem::path m_tmp_path;

    bool m_canceled = false;
};
