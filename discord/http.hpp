#pragma once
#include <cpr/cpr.h>
#include <functional>
#include <future>
#include <string>
#include <unordered_map>
#include <memory>

template<typename F>
void fire_and_forget(F &&func) {
    auto ptr = std::make_shared<std::future<void>>();
    *ptr = std::async(std::launch::async, [ptr, func]() {
        func();
    });
}

class HTTPClient {
public:
    HTTPClient(std::string api_base);

    void SetAuth(std::string auth);
    void MakeDELETE(std::string path, std::function<void(cpr::Response r)> cb);
    void MakeGET(std::string path, std::function<void(cpr::Response r)> cb);
    void MakePATCH(std::string path, std::string payload, std::function<void(cpr::Response r)> cb);
    void MakePOST(std::string path, std::string payload, std::function<void(cpr::Response r)> cb);

private:
    void OnResponse(cpr::Response r, std::function<void(cpr::Response r)> cb);
    void CleanupFutures();

    std::vector<std::future<void>> m_futures;
    std::string m_api_base;
    std::string m_authorization;
};
