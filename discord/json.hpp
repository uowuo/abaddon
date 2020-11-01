#pragma once
#include <nlohmann/json.hpp>
#include <optional>

namespace detail { // more or less because idk what to name this stuff
template<typename T>
struct is_optional : ::std::false_type {};

template<typename T>
struct is_optional<::std::optional<T>> : ::std::true_type {};

template<typename T>
inline void json_direct(const ::nlohmann::json &j, const char *key, T &val) {
    if constexpr (is_optional<T>::value)
        val = j.at(key).get<T::value_type>();
    else
        j.at(key).get_to(val);
}

template<typename T>
inline void json_optional(const ::nlohmann::json &j, const char *key, T &val) {
    if constexpr (is_optional<T>::value) {
        if (j.contains(key))
            val = j.at(key).get<T::value_type>();
        else
            val = ::std::nullopt;
    } else {
        if (j.contains(key))
            j.at(key).get_to(val);
    }
}

template<typename T>
inline void json_nullable(const ::nlohmann::json &j, const char *key, T &val) {
    if constexpr (is_optional<T>::value) {
        const auto &at = j.at(key);
        if (!at.is_null())
            val = at.get<T::value_type>();
        else
            val = std::nullopt;
    } else {
        const auto &at = j.at(key);
        if (!at.is_null())
            at.get_to(val);
    }
}

template<typename T>
inline void json_optional_nullable(const ::nlohmann::json &j, const char *key, T &val) {
    if constexpr (is_optional<T>::value) {
        if (j.contains(key)) {
            const auto &at = j.at(key);
            if (!at.is_null())
                val = at.get<T::value_type>();
            else
                val = std::nullopt;
        } else {
            val = std::nullopt;
        }
    } else {
        if (j.contains(key)) {
            const auto &at = j.at(key);
            if (!at.is_null())
                at.get_to(val);
        }
    }
}

template<typename T>
inline void json_update_optional_nullable(const ::nlohmann::json &j, const char *key, T &val) {
    if constexpr (is_optional<T>::value) {
        if (j.contains(key)) {
            const auto &at = j.at(key);
            if (!at.is_null())
                val = at.get<T::value_type>();
            else
                val = std::nullopt;
        }
    } else {
        if (j.contains(key)) {
            const auto &at = j.at(key);
            if (!at.is_null())
                at.get_to(val);
            else
                val = T();
        }
    }
}

template<typename T, typename U>
inline void json_update_optional_nullable_default(const ::nlohmann::json &j, const char *key, T &val, const U &fallback) {
    if constexpr (is_optional<T>::value) {
        if (j.contains(key)) {
            const auto &at = j.at(key);
            if (at.is_null())
                val = fallback;
            else
                val = at.get<T::value_type>();
        }
    } else {
        if (j.contains(key)) {
            const auto &at = j.at(key);
            if (at.is_null())
                val = fallback;
            else
                at.get_to(val);
        }
    }
}
} // namespace detail

#define JS_D(k, t)                    \
    do {                              \
        detail::json_direct(j, k, t); \
    } while (0)

#define JS_O(k, t)                      \
    do {                                \
        detail::json_optional(j, k, t); \
    } while (0)

#define JS_N(k, t)                      \
    do {                                \
        detail::json_nullable(j, k, t); \
    } while (0)

#define JS_ON(k, t)                              \
    do {                                         \
        detail::json_optional_nullable(j, k, t); \
    } while (0)

#define JS_RD(k, t)                                     \
    do {                                                \
        detail::json_update_optional_nullable(j, k, t); \
    } while (0)

#define JS_RV(k, t, d)                                             \
    do {                                                           \
        detail::json_update_optional_nullable_default(j, k, t, d); \
    } while (0)
