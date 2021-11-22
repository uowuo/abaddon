#pragma once
#include <functional>
#include <future>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <queue>
#include <glibmm.h>
#include "http.hpp"

class HTTPClient {
public:
    HTTPClient();

    void SetBase(const std::string &url);

    void SetUserAgent(std::string agent);
    void SetAuth(std::string auth);
    void MakeDELETE(const std::string &path, std::function<void(http::response_type r)> cb);
    void MakeGET(const std::string &path, std::function<void(http::response_type r)> cb);
    void MakePATCH(const std::string &path, const std::string &payload, std::function<void(http::response_type r)> cb);
    void MakePOST(const std::string &path, const std::string &payload, std::function<void(http::response_type r)> cb);
    void MakePUT(const std::string &path, const std::string &payload, std::function<void(http::response_type r)> cb);

private:
    void OnResponse(const http::response_type &r, std::function<void(http::response_type r)> cb);
    void CleanupFutures();

    mutable std::mutex m_mutex;
    Glib::Dispatcher m_dispatcher;
    std::queue<std::function<void()>> m_queue;
    void RunCallbacks();

    std::vector<std::future<void>> m_futures;
    std::string m_api_base;
    std::string m_authorization;
    std::string m_agent;
};
