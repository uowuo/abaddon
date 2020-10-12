#pragma once
#include <cpr/cpr.h>
#include <functional>
#include <future>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <queue>
#include <glibmm.h>

class HTTPClient {
public:
    HTTPClient(std::string api_base);

    void SetAuth(std::string auth);
    void MakeDELETE(std::string path, std::function<void(cpr::Response r)> cb);
    void MakeGET(std::string path, std::function<void(cpr::Response r)> cb);
    void MakePATCH(std::string path, std::string payload, std::function<void(cpr::Response r)> cb);
    void MakePOST(std::string path, std::string payload, std::function<void(cpr::Response r)> cb);
    void MakePUT(std::string path, std::string payload, std::function<void(cpr::Response r)> cb);

private:
    void OnResponse(cpr::Response r, std::function<void(cpr::Response r)> cb);
    void CleanupFutures();

    mutable std::mutex m_mutex;
    Glib::Dispatcher m_dispatcher;
    std::queue<std::function<void()>> m_queue;
    void RunCallbacks();

    std::vector<std::future<void>> m_futures;
    std::string m_api_base;
    std::string m_authorization;
};
