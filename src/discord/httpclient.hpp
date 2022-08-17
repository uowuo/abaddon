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
    void SetPersistentHeader(std::string name, std::string value);
    void SetCookie(std::string_view cookie);

    void MakeDELETE(const std::string &path, const std::function<void(http::response_type r)> &cb);
    void MakeGET(const std::string &path, const std::function<void(http::response_type r)> &cb);
    void MakePATCH(const std::string &path, const std::string &payload, const std::function<void(http::response_type r)> &cb);
    void MakePOST(const std::string &path, const std::string &payload, const std::function<void(http::response_type r)> &cb);
    void MakePUT(const std::string &path, const std::string &payload, const std::function<void(http::response_type r)> &cb);

    [[nodiscard]] http::request CreateRequest(http::EMethod method, std::string path);
    void Execute(http::request &&req, const std::function<void(http::response_type r)> &cb);

private:
    void AddHeaders(http::request &r);

    void OnResponse(const http::response_type &r, const std::function<void(http::response_type r)> &cb);
    void CleanupFutures();

    mutable std::mutex m_mutex;
    Glib::Dispatcher m_dispatcher;
    std::queue<std::function<void()>> m_queue;
    void RunCallbacks();

    std::vector<std::future<void>> m_futures;
    std::string m_api_base;
    std::string m_authorization;
    std::string m_agent;
    std::unordered_map<std::string, std::string> m_headers;
    std::string m_cookie;
};
