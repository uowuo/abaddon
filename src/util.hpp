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

#include <sigc++/slot.h>

namespace Glib {
class ustring;
}

namespace Gdk {
class RGBA;
}

namespace Gtk {
class Widget;
class Menu;
class ListBox;
} // namespace Gtk

#define NOOP_CALLBACK [](...) {}

namespace util {
bool IsFolder(std::string_view path);

bool IsFile(std::string_view path);

uint64_t TimeToEpoch(int year, int month, int day, int hour, int minute, int seconds);
} // namespace util

void LaunchBrowser(const Glib::ustring &url);
void GetImageDimensions(int inw, int inh, int &outw, int &outh, int clampw = 400, int clamph = 300);
std::string IntToCSSColor(int color);
Gdk::RGBA IntToRGBA(int color);
void AddWidgetMenuHandler(Gtk::Widget *widget, Gtk::Menu &menu, const sigc::slot<void()> &pre_callback);
std::vector<std::string> StringSplit(const std::string &str, const char *delim);
std::string GetExtension(std::string url);
bool IsURLViewableImage(const std::string &url);
std::vector<uint8_t> ReadWholeFile(const std::string &path);
std::string HumanReadableBytes(uint64_t bytes);
std::string FormatISO8601(const std::string &in, int extra_offset = 0, const std::string &fmt = "%x %X");
void AddPointerCursor(Gtk::Widget &widget);

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

void ScrollListBoxToSelected(Gtk::ListBox &list);

bool StringContainsCaseless(const Glib::ustring &str, const Glib::ustring &sub);
