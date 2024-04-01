#include "startup.hpp"

#include <future>
#include <memory>

#include "abaddon.hpp"

DiscordStartupDialog::DiscordStartupDialog(Gtk::Window &window)
    : Gtk::MessageDialog(window, "", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_NONE, true) {
    m_dispatcher.connect(sigc::mem_fun(*this, &DiscordStartupDialog::DispatchCallback));

    property_text() = "Getting connection info...";

    RunAsync();
}

std::optional<std::string> DiscordStartupDialog::GetCookie() const {
    return m_cookie;
}

std::optional<uint32_t> DiscordStartupDialog::GetBuildNumber() const {
    return m_build_number;
}

// good enough
std::optional<std::pair<std::string, std::string>> ParseCookie(const Glib::ustring &str) {
    auto regex = Glib::Regex::create("\\t");
    const std::vector<Glib::ustring> split = regex->split(str);
    if (split.size() < 7) return {};

    return { { split[5], split[6] } };
}

std::optional<Glib::ustring> GetJavascriptFileFromAppPage(const Glib::ustring &contents) {
    auto regex = Glib::Regex::create(R"(/assets/\w+\.?\w{20}\.js)");
    std::vector<Glib::ustring> matches;

    // regex->match_all doesnt work for some reason
    int start_position = 0;
    Glib::MatchInfo match;
    while (regex->match(contents, start_position, match)) {
        const auto str = match.fetch(0);
        matches.push_back(str);
        int foo;
        match.fetch_pos(0, start_position, foo);
        start_position += str.size();
    }

    if (matches.size() >= 7) {
        return matches[matches.size() - 7];
    }

    return {};
}

std::optional<uint32_t> GetBuildNumberFromJSURL(const Glib::ustring &url, const std::string &cookie) {
    http::request req(http::REQUEST_GET, "https://discord.com" + url);
    req.set_header("Accept-Language", "en-US,en;q=0.9");
    req.set_header("Sec-Fetch-Dest", "document");
    req.set_header("Sec-Fetch-Mode", "navigate");
    req.set_header("Sec-Fetch-Site", "none");
    req.set_header("Sec-Fetch-User", "?1");
    req.set_user_agent(Abaddon::Get().GetSettings().UserAgent);

    curl_easy_setopt(req.get_curl(), CURLOPT_COOKIE, cookie.c_str());

    auto res = req.execute();
    if (res.error) return {};

    auto regex = Glib::Regex::create(R"(Build Number: "\).concat\("(\d+))");
    Glib::MatchInfo match;
    Glib::ustring string = res.text;
    if (regex->match(string, match)) {
        const auto str_value = match.fetch(1);
        try {
            return std::stoul(str_value);
        } catch (...) { return {}; }
    }

    return {};
}

std::pair<std::optional<std::string>, std::string> GetCookieTask() {
    http::request req(http::REQUEST_GET, "https://discord.com/app");
    req.set_header("Accept-Language", "en-US,en;q=0.9");
    req.set_header("Sec-Fetch-Dest", "document");
    req.set_header("Sec-Fetch-Mode", "navigate");
    req.set_header("Sec-Fetch-Site", "none");
    req.set_header("Sec-Fetch-User", "?1");
    req.set_user_agent(Abaddon::Get().GetSettings().UserAgent);

    curl_easy_setopt(req.get_curl(), CURLOPT_COOKIEFILE, "");

    auto res = req.execute();
    if (res.error) return {};

    curl_slist *slist;
    if (curl_easy_getinfo(req.get_curl(), CURLINFO_COOKIELIST, &slist) != CURLE_OK) {
        return {};
    }

    std::string dcfduid;
    std::string sdcfduid;

    for (auto *cur = slist; cur != nullptr; cur = cur->next) {
        const auto cookie = ParseCookie(cur->data);
        if (cookie.has_value()) {
            if (cookie->first == "__dcfduid") {
                dcfduid = cookie->second;
            } else if (cookie->first == "__sdcfduid") {
                sdcfduid = cookie->second;
            }
        }
    }
    curl_slist_free_all(slist);

    if (!dcfduid.empty() && !sdcfduid.empty()) {
        return { "__dcfduid=" + dcfduid + "; __sdcfduid=" + sdcfduid, res.text };
    }

    return {};
}

void DiscordStartupDialog::RunAsync() {
    auto futptr = std::make_shared<std::future<void>>();
    *futptr = std::async(std::launch::async, [this, futptr] {
        auto [opt_cookie, app_page] = GetCookieTask();
        m_cookie = opt_cookie;
        if (opt_cookie.has_value()) {
            auto js_url = GetJavascriptFileFromAppPage(app_page);
            if (js_url.has_value()) {
                m_build_number = GetBuildNumberFromJSURL(*js_url, *opt_cookie);
            }
        }
        m_dispatcher.emit();
    });
}

void DiscordStartupDialog::DispatchCallback() {
    response(Gtk::RESPONSE_OK);
}
