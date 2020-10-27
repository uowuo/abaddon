#pragma once
#include <cpr/cpr.h>
#include <functional>
#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include "util.hpp"

// todo throttle requests and keep track of active requests to stop redundant requests

class Cache {
public:
    Cache();
    ~Cache();

    using callback_type = std::function<void(std::string)>;
    void GetFileFromURL(std::string url, callback_type cb);
    std::string GetPathIfCached(std::string url);

private:
    std::string SanitizeString(std::string str);
    void CleanupFutures();
    void RespondFromPath(std::filesystem::path path, callback_type cb);
    void OnResponse(const cpr::Response &r);

    std::unique_ptr<Semaphore> m_semaphore;

    std::unordered_map<std::string, std::vector<callback_type>> m_callbacks;
    std::vector<std::future<void>> m_futures;
    std::filesystem::path m_tmp_path;

    bool m_canceled = false;
};
