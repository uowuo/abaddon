#pragma once
#include <type_traits>

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
