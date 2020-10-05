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

// https://www.compuphase.com/cmetric.htm
inline double ColorDistance(int c1, int c2) {
    int r1 = (c1 & 0xFF0000) >> 16;
    int g1 = (c1 & 0x00FF00) >> 8;
    int b1 = (c1 & 0x0000FF) >> 0;
    int r2 = (c2 & 0xFF0000) >> 16;
    int g2 = (c2 & 0x00FF00) >> 8;
    int b2 = (c2 & 0x0000FF) >> 0;

    int rmean = (r1 - r2) / 2;
    int r = r1 - r2;
    int g = g1 - g2;
    int b = b1 - b2;
    return sqrt((((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8));
}

// somehow this works
template<typename F>
std::string RegexReplaceMany(std::string str, std::string regexstr, F func) {
    std::regex reg(regexstr, std::regex_constants::ECMAScript);
    std::smatch match;

    std::vector<std::tuple<int, int, std::string, int>> matches;

    std::string::const_iterator sstart(str.cbegin());
    while (std::regex_search(sstart, str.cend(), match, reg)) {
        const auto start = std::distance(str.cbegin(), sstart) + match.position();
        const auto end = start + match.length();
        matches.push_back(std::make_tuple(static_cast<int>(start), static_cast<int>(end), match[1].str(), static_cast<int>(match.str().size())));
        sstart = match.suffix().first;
    }

    int offset = 0;
    for (auto it = matches.begin(); it != matches.end(); it++) {
        const auto start = std::get<0>(*it);
        const auto end = std::get<1>(*it);
        const auto match = std::get<2>(*it);
        const auto full_match_size = std::get<3>(*it);
        const auto replacement = func(match);
        const auto diff = full_match_size - replacement.size();

        str = str.substr(0, start) + replacement + str.substr(end);

        offset += diff;

        std::get<0>(*(it + 1)) -= offset;
        std::get<1>(*(it + 1)) -= offset;
    }

    return str;
}
