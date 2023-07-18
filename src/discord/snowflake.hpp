#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <glibmm/ustring.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

struct Snowflake {
    Snowflake();
    Snowflake(uint64_t n);
    Snowflake(const std::string &str);
    Snowflake(const Glib::ustring &str);

    static Snowflake FromNow(); // not thread safe
    static Snowflake FromISO8601(std::string_view ts);

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] Glib::ustring GetLocalTimestamp() const;
    [[nodiscard]] uint64_t GetUnixMilliseconds() const noexcept;

    bool operator==(const Snowflake &s) const noexcept {
        return m_num == s.m_num;
    }

    bool operator<(const Snowflake &s) const noexcept {
        return m_num < s.m_num;
    }

    operator uint64_t() const noexcept {
        return m_num;
    }

    const static Snowflake Invalid;                        // makes sense to me
    const static uint64_t SecondsInterval = 4194304000ULL; // the "difference" between two snowflakes one second apart

    friend void from_json(const nlohmann::json &j, Snowflake &s);
    friend void to_json(nlohmann::json &j, const Snowflake &s);

private:
    friend struct std::hash<Snowflake>;
    friend struct std::less<Snowflake>;
    unsigned long long m_num;
};

template<>
struct fmt::formatter<Snowflake> : fmt::formatter<std::string> {
    auto format(Snowflake id, format_context &ctx) -> decltype(ctx.out()) {
        return format_to(ctx.out(), "[id: {}]", static_cast<uint64_t>(id));
    }
};

namespace std {
template<>
struct hash<Snowflake> {
    std::size_t operator()(const Snowflake &k) const {
        return k.m_num;
    }
};

template<>
struct less<Snowflake> {
    bool operator()(const Snowflake &l, const Snowflake &r) const {
        return l.m_num < r.m_num;
    }
};
} // namespace std
