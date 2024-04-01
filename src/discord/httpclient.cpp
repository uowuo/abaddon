#include "httpclient.hpp"

#include <utility>

HTTPClient::HTTPClient() {
    m_dispatcher.connect(sigc::mem_fun(*this, &HTTPClient::RunCallbacks));
}

void HTTPClient::SetBase(const std::string &url) {
    m_api_base = url;
}

void HTTPClient::SetUserAgent(std::string agent) {
    m_agent = std::move(agent);
}

void HTTPClient::SetAuth(std::string auth) {
    m_authorization = std::move(auth);
}

void HTTPClient::SetPersistentHeader(std::string name, std::string value) {
    m_headers.insert_or_assign(std::move(name), std::move(value));
}

void HTTPClient::SetCookie(std::string_view cookie) {
    m_cookie = cookie;
}

void HTTPClient::MakeDELETE(const std::string &path, const std::function<void(http::response_type r)> &cb) {
    printf("DELETE %s\n", path.c_str());
    m_futures.push_back(std::async(std::launch::async, [this, path, cb] {
        http::request req(http::REQUEST_DELETE, m_api_base + path);
        AddHeaders(req);
        req.set_header("Authorization", m_authorization);
        req.set_header("Origin", "https://discord.com");
        req.set_user_agent(!m_agent.empty() ? m_agent : "Abaddon");

        auto res = req.execute();

        OnResponse(res, cb);
    }));
}

void HTTPClient::MakePATCH(const std::string &path, const std::string &payload, const std::function<void(http::response_type r)> &cb) {
    printf("PATCH %s\n", path.c_str());
    m_futures.push_back(std::async(std::launch::async, [this, path, cb, payload] {
        http::request req(http::REQUEST_PATCH, m_api_base + path);
        AddHeaders(req);
        req.set_header("Authorization", m_authorization);
        req.set_header("Content-Type", "application/json");
        req.set_header("Origin", "https://discord.com");
        req.set_user_agent(!m_agent.empty() ? m_agent : "Abaddon");
        req.set_body(payload);

        auto res = req.execute();

        OnResponse(res, cb);
    }));
}

void HTTPClient::MakePOST(const std::string &path, const std::string &payload, const std::function<void(http::response_type r)> &cb) {
    printf("POST %s\n", path.c_str());
    m_futures.push_back(std::async(std::launch::async, [this, path, cb, payload] {
        http::request req(http::REQUEST_POST, m_api_base + path);
        AddHeaders(req);
        req.set_header("Authorization", m_authorization);
        req.set_header("Content-Type", "application/json");
        req.set_header("Origin", "https://discord.com");
        req.set_user_agent(!m_agent.empty() ? m_agent : "Abaddon");
        req.set_body(payload);

        auto res = req.execute();

        OnResponse(res, cb);
    }));
}

void HTTPClient::MakePUT(const std::string &path, const std::string &payload, const std::function<void(http::response_type r)> &cb) {
    printf("PUT %s\n", path.c_str());
    m_futures.push_back(std::async(std::launch::async, [this, path, cb, payload] {
        http::request req(http::REQUEST_PUT, m_api_base + path);
        AddHeaders(req);
        req.set_header("Authorization", m_authorization);
        req.set_header("Origin", "https://discord.com");
        if (!payload.empty())
            req.set_header("Content-Type", "application/json");
        req.set_user_agent(!m_agent.empty() ? m_agent : "Abaddon");
        req.set_body(payload);

        auto res = req.execute();

        OnResponse(res, cb);
    }));
}

void HTTPClient::MakeGET(const std::string &path, const std::function<void(http::response_type r)> &cb) {
    printf("GET %s\n", path.c_str());
    m_futures.push_back(std::async(std::launch::async, [this, path, cb] {
        http::request req(http::REQUEST_GET, m_api_base + path);
        AddHeaders(req);
        req.set_header("Authorization", m_authorization);
        req.set_user_agent(!m_agent.empty() ? m_agent : "Abaddon");

        auto res = req.execute();

        OnResponse(res, cb);
    }));
}

http::request HTTPClient::CreateRequest(http::EMethod method, std::string path) {
    http::request req(method, m_api_base + path);
    req.set_header("Authorization", m_authorization);
    req.set_user_agent(!m_agent.empty() ? m_agent : "Abaddon");
    return req;
}

void HTTPClient::Execute(http::request &&req, const std::function<void(http::response_type r)> &cb) {
    printf("%s %s\n", req.get_method(), req.get_url().c_str());
    m_futures.push_back(std::async(std::launch::async, [this, cb, req = std::move(req)]() mutable {
        auto res = req.execute();
        OnResponse(res, cb);
    }));
}

void HTTPClient::CleanupFutures() {
    for (auto it = m_futures.begin(); it != m_futures.end();) {
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            it = m_futures.erase(it);
        else
            it++;
    }
}

void HTTPClient::RunCallbacks() {
    m_mutex.lock();
    m_queue.front()();
    m_queue.pop();
    m_mutex.unlock();
}

void HTTPClient::AddHeaders(http::request &r) {
    for (const auto &[name, val] : m_headers) {
        r.set_header(name, val);
    }
    curl_easy_setopt(r.get_curl(), CURLOPT_COOKIE, m_cookie.c_str());
    // https://github.com/curl/curl/issues/13226
    // TODO remove when new release
#if defined(LIBCURL_VERSION_NUM) && (LIBCURL_VERSION_NUM == 0x080701 || LIBCURL_VERSION_NUM == 0x080700)
    //
#else
    curl_easy_setopt(r.get_curl(), CURLOPT_ACCEPT_ENCODING, "");
#endif
}

void HTTPClient::OnResponse(const http::response_type &r, const std::function<void(http::response_type r)> &cb) {
    CleanupFutures();
    try {
        m_mutex.lock();
        m_queue.push([r, cb] { cb(r); });
        m_dispatcher.emit();
        m_mutex.unlock();
    } catch (const std::exception &e) {
        fprintf(stderr, "error handling response (%s, code %d): %s\n", r.url.c_str(), r.status_code, e.what());
    }
}
