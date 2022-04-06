#pragma once
#include <string>
#include <curl/curl.h>

// i regret not using snake case for everything oh well

namespace http {
enum EStatusCode : int {
    Continue = 100,
    SwitchingProtocols = 101,
    Processing = 102,
    EarlyHints = 103,

    OK = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultiStatus = 207,
    AlreadyReported = 208,
    IMUsed = 226,

    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    SwitchProxy = 306,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,

    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthorizationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    PayloadTooLarge = 413,
    URITooLong = 414,
    UnsupportedMediaType = 415,
    RangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    ImATeapot = 418,
    MisdirectedRequest = 421,
    UnprocessableEntity = 422,
    Locked = 423,
    FailedDependency = 424,
    TooEarly = 425,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    UnavailableForLegalReasons = 451,

    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505,
    VariantAlsoNegotiates = 506,
    InsufficientStorage = 507,
    LoopDetected = 508,
    NotExtended = 510,
    NetworkAuthenticationRequired = 511,

    ClientError = 1,
    ClientErrorCURLInit,
    ClientErrorCURLPerform,
    ClientErrorMax = 99,
};

enum EMethod {
    REQUEST_GET,
    REQUEST_POST,
    REQUEST_PATCH,
    REQUEST_PUT,
    REQUEST_DELETE,
};

struct response {
    EStatusCode status_code;
    std::string text;
    std::string url;
    bool error = false;
    std::string error_string;
};

struct request {
    request(EMethod method, std::string url);
    ~request();

    void set_verify_ssl(bool verify);
    void set_proxy(const std::string &proxy);
    void set_header(const std::string &name, const std::string &value);
    void set_body(const std::string &data);
    void set_user_agent(const std::string &data);

    response execute();

private:
    void prepare();

    CURL *m_curl;
    std::string m_url;
    const char *m_method;
    curl_slist *m_header_list = nullptr;
    char m_error_buf[CURL_ERROR_SIZE] = { 0 };
};

using response_type = response;

namespace detail {
    size_t curl_write_data_callback(void *ptr, size_t size, size_t nmemb, void *userdata);

    response make_response(const std::string &url, int code);
    void check_init();
} // namespace detail
} // namespace http
