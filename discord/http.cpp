#include "http.hpp"

HTTPClient::HTTPClient(std::string api_base)
    : m_api_base(api_base) {}

void HTTPClient::SetAuth(std::string auth) {
    m_authorization = auth;
}

void HTTPClient::MakeDELETE(std::string path, std::function<void(cpr::Response r)> cb) {
    printf("DELETE %s\n", path.c_str());
    auto url = cpr::Url { m_api_base + path };
    auto headers = cpr::Header {
        { "Authorization", m_authorization },
    };
#ifdef USE_LOCAL_PROXY
    m_futures.push_back(cpr::DeleteCallback(
        std::bind(&HTTPClient::OnResponse, this, std::placeholders::_1, cb),
        url, headers,
        cpr::Proxies { { "http", "127.0.0.1:8888" }, { "https", "127.0.0.1:8888" } },
        cpr::VerifySsl { false }));
#else
    m_futures.push_back(cpr::DeleteCallback(
        std::bind(&HTTPClient::OnResponse, this, std::placeholders::_1, cb),
        url, headers));
#endif
}

void HTTPClient::MakePATCH(std::string path, std::string payload, std::function<void(cpr::Response r)> cb) {
    printf("PATCH %s\n", path.c_str());
    auto url = cpr::Url { m_api_base + path };
    auto headers = cpr::Header {
        { "Authorization", m_authorization },
        { "Content-Type", "application/json" },
    };
    auto body = cpr::Body { payload };
#ifdef USE_LOCAL_PROXY
    m_futures.push_back(cpr::PatchCallback(
        std::bind(&HTTPClient::OnResponse, this, std::placeholders::_1, cb),
        url, headers, body,
        cpr::Proxies { { "http", "127.0.0.1:8888" }, { "https", "127.0.0.1:8888" } },
        cpr::VerifySsl { false }));
#else
    m_futures.push_back(cpr::PatchCallback(
        std::bind(&HTTPClient::OnResponse, this, std::placeholders::_1, cb),
        url, headers, body));
#endif
}

void HTTPClient::MakePOST(std::string path, std::string payload, std::function<void(cpr::Response r)> cb) {
    printf("POST %s\n", path.c_str());
    auto url = cpr::Url { m_api_base + path };
    auto headers = cpr::Header {
        { "Authorization", m_authorization },
        { "Content-Type", "application/json" },
    };
    auto body = cpr::Body { payload };
#ifdef USE_LOCAL_PROXY
    m_futures.push_back(cpr::PostCallback(
        std::bind(&HTTPClient::OnResponse, this, std::placeholders::_1, cb),
        url, headers, body,
        cpr::Proxies { { "http", "127.0.0.1:8888" }, { "https", "127.0.0.1:8888" } },
        cpr::VerifySsl { false }));
#else
    m_futures.push_back(cpr::PostCallback(
        std::bind(&HTTPClient::OnResponse, this, std::placeholders::_1, cb),
        url, headers, body));
#endif
}

void HTTPClient::MakeGET(std::string path, std::function<void(cpr::Response r)> cb) {
    printf("GET %s\n", path.c_str());
    auto url = cpr::Url { m_api_base + path };
    auto headers = cpr::Header {
        { "Authorization", m_authorization },
        { "Content-Type", "application/json" },
    };
#ifdef USE_LOCAL_PROXY
    m_futures.push_back(cpr::GetCallback(
        std::bind(&HTTPClient::OnResponse, this, std::placeholders::_1, cb),
        url, headers,
        cpr::Proxies { { "http", "127.0.0.1:8888" }, { "https", "127.0.0.1:8888" } },
        cpr::VerifySsl { false }));
#else
    m_futures.push_back(cpr::GetCallback(
        std::bind(&HTTPClient::OnResponse, this, std::placeholders::_1, cb),
        url, headers));
#endif
}

void HTTPClient::CleanupFutures() {
    for (auto it = m_futures.begin(); it != m_futures.end();) {
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            it = m_futures.erase(it);
        else
            it++;
    }
}

void HTTPClient::OnResponse(cpr::Response r, std::function<void(cpr::Response r)> cb) {
    CleanupFutures();
    try {
        cb(r);
    } catch (std::exception &e) {
        fprintf(stderr, "error handling response (%s, code %d): %s\n", r.url.c_str(), r.status_code, e.what());
    }
}
