#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include "misc/is_optional.hpp"

namespace detail { // more or less because idk what to name this stuff
template<typename T>
inline void json_direct(const nlohmann::json &j, const char *key, T &val) {
    if constexpr (is_optional<T>::value)
        val = j.at(key).get<typename T::value_type>();
    else
        j.at(key).get_to(val);
}

template<typename T>
inline void json_optional(const nlohmann::json &j, const char *key, T &val) {
    if constexpr (is_optional<T>::value) {
        if (j.contains(key))
            val = j.at(key).get<typename T::value_type>();
        else
            val = std::nullopt;
    } else {
        if (j.contains(key))
            j.at(key).get_to(val);
    }
}

template<typename T>
inline void json_nullable(const nlohmann::json &j, const char *key, T &val) {
    if constexpr (is_optional<T>::value) {
        const auto &at = j.at(key);
        if (!at.is_null())
            val = at.get<typename T::value_type>();
        else
            val = std::nullopt;
    } else {
        const auto &at = j.at(key);
        if (!at.is_null())
            at.get_to(val);
    }
}

template<typename T>
inline void json_optional_nullable(const nlohmann::json &j, const char *key, T &val) {
    if constexpr (is_optional<T>::value) {
        if (j.contains(key)) {
            const auto &at = j.at(key);
            if (!at.is_null())
                val = at.get<typename T::value_type>();
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
inline void json_update_optional_nullable(const nlohmann::json &j, const char *key, T &val) {
    if constexpr (is_optional<T>::value) {
        if (j.contains(key)) {
            const auto &at = j.at(key);
            if (!at.is_null())
                val = at.get<typename T::value_type>();
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
inline void json_update_optional_nullable_default(const nlohmann::json &j, const char *key, T &val, const U &fallback) {
    if constexpr (is_optional<T>::value) {
        if (j.contains(key)) {
            const auto &at = j.at(key);
            if (at.is_null())
                val = fallback;
            else
                val = at.get<typename T::value_type>();
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

// get a json value that is guaranteed to be present and non-null
#define JS_D(k, t)                    \
    do {                              \
        detail::json_direct(j, k, t); \
    } while (0)

// get a json value that may not be present
#define JS_O(k, t)                      \
    do {                                \
        detail::json_optional(j, k, t); \
    } while (0)

// get a json value that may be null
#define JS_N(k, t)                      \
    do {                                \
        detail::json_nullable(j, k, t); \
    } while (0)

// get a json value that may not be present or may be null
#define JS_ON(k, t)                              \
    do {                                         \
        detail::json_optional_nullable(j, k, t); \
    } while (0)

// set from a json value only if it is present. null will assign default-constructed value
#define JS_RD(k, t)                                     \
    do {                                                \
        detail::json_update_optional_nullable(j, k, t); \
    } while (0)

// set from a json value only if it is present. null will assign the given default
#define JS_RV(k, t, d)                                             \
    do {                                                           \
        detail::json_update_optional_nullable_default(j, k, t, d); \
    } while (0)

// set a json value from a std::optional only if it has a value
#define JS_IF(k, v)          \
    do {                     \
        if ((v).has_value()) \
            j[k] = *(v);     \
    } while (0)
