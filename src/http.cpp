#include "http.hpp"

#include <utility>

// #define USE_LOCAL_PROXY

namespace http {
request::request(EMethod method, std::string url)
    : m_url(std::move(url)) {
    switch (method) {
        case REQUEST_GET:
            m_method = "GET";
            break;
        case REQUEST_POST:
            m_method = "POST";
            break;
        case REQUEST_PATCH:
            m_method = "PATCH";
            break;
        case REQUEST_PUT:
            m_method = "PUT";
            break;
        case REQUEST_DELETE:
            m_method = "DELETE";
            break;
        default:
            m_method = "GET";
            break;
    }

    prepare();
}

request::request(request &&other) noexcept
    : m_curl(std::exchange(other.m_curl, nullptr))
    , m_url(std::exchange(other.m_url, ""))
    , m_method(std::exchange(other.m_method, nullptr))
    , m_header_list(std::exchange(other.m_header_list, nullptr))
    , m_form(std::exchange(other.m_form, nullptr))
    , m_read_streams(std::move(other.m_read_streams))
    , m_progress_callback(std::move(other.m_progress_callback)) {
    if (m_progress_callback) {
        curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, this);
    }
}

request::~request() {
    if (m_curl != nullptr)
        curl_easy_cleanup(m_curl);

    if (m_header_list != nullptr)
        curl_slist_free_all(m_header_list);

    if (m_form != nullptr)
        curl_mime_free(m_form);
}

const std::string &request::get_url() const {
    return m_url;
}

const char *request::get_method() const {
    return m_method;
}

size_t http_req_xferinfofunc(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    if (ultotal > 0) {
        auto *req = reinterpret_cast<request *>(clientp);
        req->m_progress_callback(ultotal, ulnow);
    }

    return 0;
}

void request::set_progress_callback(std::function<void(curl_off_t, curl_off_t)> func) {
    m_progress_callback = std::move(func);
    curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, http_req_xferinfofunc);
    curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, this);
}

void request::set_verify_ssl(bool verify) {
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, verify ? 1L : 0L);
}

void request::set_proxy(const std::string &proxy) {
    curl_easy_setopt(m_curl, CURLOPT_PROXY, proxy.c_str());
}

void request::set_header(const std::string &name, const std::string &value) {
    m_header_list = curl_slist_append(m_header_list, (name + ": " + value).c_str());
}

void request::set_body(const std::string &data) {
    curl_easy_setopt(m_curl, CURLOPT_COPYPOSTFIELDS, data.c_str());
}

void request::set_user_agent(const std::string &data) {
    curl_easy_setopt(m_curl, CURLOPT_USERAGENT, data.c_str());
}

CURL *request::get_curl() {
    return m_curl;
}

void request::make_form() {
    m_form = curl_mime_init(m_curl);
}

static size_t http_readfunc(char *buffer, size_t size, size_t nitems, void *arg) {
    auto stream = Glib::wrap(G_FILE_INPUT_STREAM(arg), true);
    int r = stream->read(buffer, size * nitems);
    if (r == -1) {
        // https://github.com/curl/curl/blob/ad9bc5976d6661cd5b03ebc379313bf657701c14/lib/mime.c#L724
        return size_t(-1);
    }
    return r;
}

// file must exist until request completes
void request::add_file(std::string_view name, const Glib::RefPtr<Gio::File> &file, std::string_view filename) {
    if (!file->query_exists()) return;

    auto *field = curl_mime_addpart(m_form);
    curl_mime_name(field, name.data());
    auto info = file->query_info();
    auto stream = file->read();
    curl_mime_data_cb(field, info->get_size(), http_readfunc, nullptr, nullptr, stream->gobj());
    curl_mime_filename(field, filename.data());

    // hold ref
    m_read_streams.insert(stream);
}

// copied
void request::add_field(std::string_view name, const char *data, size_t size) {
    auto *field = curl_mime_addpart(m_form);
    curl_mime_name(field, name.data());
    curl_mime_data(field, data, size);
}

response request::execute() {
    if (m_curl == nullptr) {
        auto response = detail::make_response(m_url, EStatusCode::ClientErrorCURLInit);
        response.error_string = "curl pointer is null";
    }

    detail::check_init();

    std::string str;
#ifdef USE_LOCAL_PROXY
    set_proxy("http://127.0.0.1:8888");
    set_verify_ssl(false);
#endif
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, m_method);
    curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, detail::curl_write_data_callback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &str);
    if (m_header_list != nullptr)
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_header_list);
    if (m_form != nullptr)
        curl_easy_setopt(m_curl, CURLOPT_MIMEPOST, m_form);

    CURLcode result = curl_easy_perform(m_curl);
    if (result != CURLE_OK) {
        auto response = detail::make_response(m_url, EStatusCode::ClientErrorCURLPerform);
        response.error_string = curl_easy_strerror(result);
        return response;
    }

    long response_code = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);

    auto response = detail::make_response(m_url, response_code);
    response.text = str;

    return response;
}

void request::prepare() {
    m_curl = curl_easy_init();
}

namespace detail {
    size_t curl_write_data_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
        const size_t n = size * nmemb;
        static_cast<std::string *>(userdata)->append(static_cast<char *>(ptr), n);
        return n;
    }

    response make_response(const std::string &url, int code) {
        response r;
        r.url = url;
        r.status_code = static_cast<EStatusCode>(code);
        if (code < http::EStatusCode::ClientErrorMax)
            r.error = true;
        return r;
    }

    void check_init() {
        static bool initialized = false;
        if (!initialized) {
            curl_global_init(CURL_GLOBAL_ALL);
            initialized = true;
        }
    }
} // namespace detail
} // namespace http
