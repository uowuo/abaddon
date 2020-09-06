#pragma once
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <iomanip>

template<typename T>
inline void AlphabeticalSort(typename T start, typename T end, std::function<std::string(const typename std::iterator_traits<T>::value_type &)> get_string) {
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
