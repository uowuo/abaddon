#include "util.hpp"

#include <array>
#include <cstring>
#include <filesystem>

#include <gtkmm.h>

void LaunchBrowser(const Glib::ustring &url) {
    GError *err = nullptr;
    if (!gtk_show_uri_on_window(nullptr, url.c_str(), GDK_CURRENT_TIME, &err))
        printf("failed to open uri: %s\n", err->message);
}

void GetImageDimensions(int inw, int inh, int &outw, int &outh, int clampw, int clamph) {
    const auto frac = static_cast<float>(inw) / static_cast<float>(inh);

    outw = inw;
    outh = inh;

    if (outw > clampw) {
        outw = clampw;
        outh = clampw / frac;
    }

    if (outh > clamph) {
        outh = clamph;
        outw = clamph * frac;
    }
}

std::vector<uint8_t> ReadWholeFile(const std::string &path) {
    std::vector<uint8_t> ret;
    FILE *fp = std::fopen(path.c_str(), "rb");
    if (fp == nullptr)
        return ret;
    std::fseek(fp, 0, SEEK_END);
    int len = std::ftell(fp);
    std::rewind(fp);
    ret.resize(len);
    std::fread(ret.data(), 1, ret.size(), fp);
    std::fclose(fp);
    return ret;
}

std::string HumanReadableBytes(uint64_t bytes) {
    constexpr static const char *x[] = { "B", "KB", "MB", "GB", "TB" };
    int order = 0;
    while (bytes >= 1000 && order < 4) { // 4=len(x)-1
        order++;
        bytes /= 1000;
    }
    return std::to_string(bytes) + x[order];
}

int GetTimezoneOffset() {
    std::time_t secs;
    std::time(&secs);
    std::tm *tptr = std::localtime(&secs);
    std::time_t local_secs = std::mktime(tptr);
    tptr = std::gmtime(&secs);
    std::time_t gmt_secs = std::mktime(tptr);
    return static_cast<int>(local_secs - gmt_secs);
}

std::string FormatISO8601(const std::string &in, int extra_offset, const std::string &fmt) {
    int yr, mon, day, hr, min, sec, tzhr, tzmin;
    float milli;
    std::sscanf(in.c_str(), "%d-%d-%dT%d:%d:%d%f+%d:%d",
                &yr, &mon, &day, &hr, &min, &sec, &milli, &tzhr, &tzmin);
    std::tm tm {};
    tm.tm_year = yr - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = day;
    tm.tm_hour = hr;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_wday = 0;
    tm.tm_yday = 0;
    tm.tm_isdst = -1;
    int offset = GetTimezoneOffset();
    tm.tm_sec += offset + extra_offset;
    mktime(&tm);
    std::array<char, 512> tmp {};
    std::strftime(tmp.data(), sizeof(tmp), fmt.c_str(), &tm);
    return tmp.data();
}

void ScrollListBoxToSelected(Gtk::ListBox &list) {
    auto cb = [&list]() -> bool {
        const auto selected = list.get_selected_row();
        if (selected == nullptr) return false;
        int x, y;
        selected->translate_coordinates(list, 0, 0, x, y);
        if (y < 0) return false;
        const auto adj = list.get_adjustment();
        if (!adj) return false;
        int min, nat;
        selected->get_preferred_height(min, nat);
        adj->set_value(y - (adj->get_page_size() - nat) / 2.0);

        return false;
    };
    Glib::signal_idle().connect(sigc::track_obj(cb, list));
}

// surely theres a better way to do this
bool StringContainsCaseless(const Glib::ustring &str, const Glib::ustring &sub) {
    const auto regex = Glib::Regex::create(Glib::Regex::escape_string(sub), Glib::REGEX_CASELESS);
    return regex->match(str);
}

std::string IntToCSSColor(int color) {
    int r = (color & 0xFF0000) >> 16;
    int g = (color & 0x00FF00) >> 8;
    int b = (color & 0x0000FF) >> 0;
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << r
       << std::hex << std::setw(2) << std::setfill('0') << g
       << std::hex << std::setw(2) << std::setfill('0') << b;
    return ss.str();
}

Gdk::RGBA IntToRGBA(int color) {
    Gdk::RGBA ret;
    ret.set_red(((color & 0xFF0000) >> 16) / 255.0);
    ret.set_green(((color & 0x00FF00) >> 8) / 255.0);
    ret.set_blue(((color & 0x0000FF) >> 0) / 255.0);
    ret.set_alpha(255.0);
    return ret;
}

// so widgets can modify the menu before it is displayed
// maybe theres a better way to do this idk
void AddWidgetMenuHandler(Gtk::Widget *widget, Gtk::Menu &menu, const sigc::slot<void()> &pre_callback) {
    sigc::signal<void()> signal;
    signal.connect(pre_callback);
    widget->signal_button_press_event().connect([&menu, signal](GdkEventButton *ev) -> bool {
        if (ev->type == GDK_BUTTON_PRESS && ev->button == GDK_BUTTON_SECONDARY) {
            signal.emit();
            menu.popup_at_pointer(reinterpret_cast<const GdkEvent *>(ev));
            return true;
        }
        return false;
        // clang-format off
    }, false);
    // clang-format on
}

std::vector<std::string> StringSplit(const std::string &str, const char *delim) {
    std::vector<std::string> parts;
    char *token = std::strtok(const_cast<char *>(str.c_str()), delim);
    while (token != nullptr) {
        parts.emplace_back(token);
        token = std::strtok(nullptr, delim);
    }
    return parts;
}

std::string GetExtension(std::string url) {
    url = StringSplit(url, "?")[0];
    url = StringSplit(url, "/").back();
    return url.find('.') != std::string::npos ? url.substr(url.find_last_of('.')) : "";
}

bool IsURLViewableImage(const std::string &url) {
    std::string lw_url = url;
    std::transform(lw_url.begin(), lw_url.end(), lw_url.begin(), ::tolower);

    const auto ext = GetExtension(lw_url);
    static const char *exts[] = { ".jpeg",
                                  ".jpg",
                                  ".png", nullptr };
    const char *str = ext.c_str();
    for (int i = 0; exts[i] != nullptr; i++)
        if (strcmp(str, exts[i]) == 0)
            return true;
    return false;
}

void AddPointerCursor(Gtk::Widget &widget) {
    widget.signal_realize().connect([&widget]() {
        auto window = widget.get_window();
        auto display = window->get_display();
        auto cursor = Gdk::Cursor::create(display, "pointer");
        window->set_cursor(cursor);
    });
}

bool util::IsFolder(std::string_view path) {
    std::error_code ec;
    const auto status = std::filesystem::status(path, ec);
    if (ec) return false;
    return status.type() == std::filesystem::file_type::directory;
}

bool util::IsFile(std::string_view path) {
    std::error_code ec;
    const auto status = std::filesystem::status(path, ec);
    if (ec) return false;
    return status.type() == std::filesystem::file_type::regular;
}

constexpr bool IsLeapYear(int year) {
    if (year % 4 != 0) return false;
    if (year % 100 != 0) return true;
    return (year % 400) == 0;
}

constexpr static int SecsPerMinute = 60;
constexpr static int SecsPerHour = 3600;
constexpr static int SecsPerDay = 86400;
constexpr static std::array<int, 12> DaysOfMonth = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// may god smite whoever is responsible for the absolutely abominable api that is C time functions
// i shouldnt have to write this. mktime ALMOST works but it adds the current timezone offset. WHY???
uint64_t util::TimeToEpoch(int year, int month, int day, int hour, int minute, int seconds) {
    uint64_t secs = 0;
    for (int y = 1970; y < year; ++y)
        secs += (IsLeapYear(y) ? 366 : 365) * SecsPerDay;
    for (int m = 1; m < month; ++m) {
        secs += DaysOfMonth[m - 1] * SecsPerDay;
        if (m == 2 && IsLeapYear(year)) secs += SecsPerDay;
    }
    secs += (day - 1) * SecsPerDay;
    secs += hour * SecsPerHour;
    secs += minute * SecsPerMinute;
    secs += seconds;
    return secs;
}
