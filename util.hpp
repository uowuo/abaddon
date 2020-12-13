#pragma once
#include <cctype>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <iomanip>
#include <regex>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <type_traits>
#include <gtkmm.h>

namespace util {
template<typename T>
struct is_optional : ::std::false_type {};

template<typename T>
struct is_optional<::std::optional<T>> : ::std::true_type {};
} // namespace util

class Semaphore {
public:
    Semaphore(int count = 0)
        : m_count(count) {}

    inline void notify() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_count++;
        lock.unlock();
        m_cv.notify_one();
    }

    inline void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_count == 0)
            m_cv.wait(lock);
        m_count--;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    int m_count;
};
// gtkmm doesnt seem to work
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    #include <shellapi.h>
#endif

inline void LaunchBrowser(std::string url) {
#if defined(_WIN32)
    // wtf i love the win32 api now ???
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    std::system(("open " + url).c_str());
#elif defined(__linux__)
    std::system(("xdg-open " + url).c_str());
#else
    printf("can't open url on this platform\n");
#endif
}

inline void GetImageDimensions(int inw, int inh, int &outw, int &outh, int clampw = 400, int clamph = 300) {
    const auto frac = static_cast<float>(inw) / inh;

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

inline std::vector<uint8_t> ReadWholeFile(std::string path) {
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

inline std::string HumanReadableBytes(uint64_t bytes) {
    constexpr static const char *x[] = { "B", "KB", "MB", "GB", "TB" };
    int order = 0;
    while (bytes >= 1000 && order < 4) { // 4=len(x)-1
        order++;
        bytes /= 1000;
    }
    return std::to_string(bytes) + x[order];
}

template<typename T>
struct Bitwise {
    static const bool enable = false;
};

template<typename T>
typename std::enable_if<Bitwise<T>::enable, T>::type operator|(T a, T b) {
    using x = typename std::underlying_type<T>::type;
    return static_cast<T>(static_cast<x>(a) | static_cast<x>(b));
}

template<typename T>
typename std::enable_if<Bitwise<T>::enable, T>::type operator|=(T &a, T b) {
    using x = typename std::underlying_type<T>::type;
    a = static_cast<T>(static_cast<x>(a) | static_cast<x>(b));
    return a;
}

template<typename T>
typename std::enable_if<Bitwise<T>::enable, T>::type operator&(T a, T b) {
    using x = typename std::underlying_type<T>::type;
    return static_cast<T>(static_cast<x>(a) & static_cast<x>(b));
}

template<typename T>
typename std::enable_if<Bitwise<T>::enable, T>::type operator&=(T &a, T b) {
    using x = typename std::underlying_type<T>::type;
    a = static_cast<T>(static_cast<x>(a) & static_cast<x>(b));
    return a;
}

template<typename T>
typename std::enable_if<Bitwise<T>::enable, T>::type operator~(T a) {
    return static_cast<T>(~static_cast<typename std::underlying_type<T>::type>(a));
}

template<typename T>
inline void AlphabeticalSort(T start, T end, std::function<std::string(const typename std::iterator_traits<T>::value_type &)> get_string) {
    std::sort(start, end, [&](const auto &a, const auto &b) -> bool {
        const std::string &s1 = get_string(a);
        const std::string &s2 = get_string(b);

        if (s1.empty() || s2.empty())
            return s1 < s2;

        bool ac[] = {
            !isalnum(s1[0]),
            !isalnum(s2[0]),
            !!isdigit(s1[0]),
            !!isdigit(s2[0]),
            !!isalpha(s1[0]),
            !!isalpha(s2[0]),
        };

        if ((ac[0] && ac[1]) || (ac[2] && ac[3]) || (ac[4] && ac[5]))
            return s1 < s2;

        return ac[0] || ac[5];
    });
}

inline std::string IntToCSSColor(int color) {
    int r = (color & 0xFF0000) >> 16;
    int g = (color & 0x00FF00) >> 8;
    int b = (color & 0x0000FF) >> 0;
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << r
       << std::hex << std::setw(2) << std::setfill('0') << g
       << std::hex << std::setw(2) << std::setfill('0') << b;
    return ss.str();
}

inline void AddWidgetMenuHandler(Gtk::Widget *widget, Gtk::Menu &menu) {
    widget->signal_button_press_event().connect([&menu](GdkEventButton *ev) -> bool {
        if (ev->type == GDK_BUTTON_PRESS && ev->button == GDK_BUTTON_SECONDARY) {
            menu.popup_at_pointer(reinterpret_cast<const GdkEvent *>(ev));
            return true;
        }
        return false;
        // clang-format off
    }, false);
    // clang-format on
}
