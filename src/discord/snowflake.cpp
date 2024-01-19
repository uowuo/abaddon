#include "snowflake.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>

#include "util.hpp"

constexpr static uint64_t DiscordEpochSeconds = 1420070400;

const Snowflake Snowflake::Invalid = -1ULL;

Snowflake::Snowflake()
    : m_num(Invalid) {}

Snowflake::Snowflake(uint64_t n)
    : m_num(n) {}

Snowflake::Snowflake(const std::string &str) {
    if (!str.empty())
        m_num = std::stoull(str);
    else
        m_num = Invalid;
}
Snowflake::Snowflake(const Glib::ustring &str) {
    if (!str.empty())
        m_num = std::strtoull(str.c_str(), nullptr, 10);
    else
        m_num = Invalid;
}

Snowflake Snowflake::FromNow() {
    using namespace std::chrono;
    // not guaranteed to work but it probably will anyway
    static uint64_t counter = 0;
    const auto millis_since_epoch = static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    const auto epoch = millis_since_epoch - DiscordEpochSeconds * 1000;
    uint64_t snowflake = epoch << 22;
    // worker id and process id would be OR'd in here but there's no point
    snowflake |= counter++ % 4096;
    return snowflake;
}

Snowflake Snowflake::FromISO8601(std::string_view ts) {
    int yr, mon, day, hr, min, sec, tzhr, tzmin;
    float milli;
    if (std::sscanf(ts.data(), "%d-%d-%dT%d:%d:%d%f+%d:%d",
                    &yr, &mon, &day, &hr, &min, &sec, &milli, &tzhr, &tzmin) != 9) return Snowflake::Invalid;
    const auto epoch = util::TimeToEpoch(yr, mon, day, hr, min, sec);
    if (epoch < DiscordEpochSeconds) return Snowflake::Invalid;
    return SecondsInterval * (epoch - DiscordEpochSeconds) + static_cast<uint64_t>(milli * static_cast<float>(SecondsInterval));
}

bool Snowflake::IsValid() const {
    return m_num != Invalid;
}

Glib::ustring Snowflake::GetLocalTimestamp() const {
    const time_t secs_since_epoch = (m_num / SecondsInterval) + DiscordEpochSeconds;
    const std::tm tm = *localtime(&secs_since_epoch);
    std::array<char, 256> tmp {};
    std::strftime(tmp.data(), sizeof(tmp), "%X %x", &tm);
    return tmp.data();
}

uint64_t Snowflake::GetUnixMilliseconds() const noexcept {
    return (m_num >> 22) + DiscordEpochSeconds * 1000;
}

void from_json(const nlohmann::json &j, Snowflake &s) {
    if (j.is_string()) {
        std::string tmp;
        j.get_to(tmp);
        s.m_num = std::stoull(tmp);
    } else {
        j.get_to(s.m_num);
    }
}

void to_json(nlohmann::json &j, const Snowflake &s) {
    j = std::to_string(s);
}
