#include "snowflake.hpp"
#include <ctime>
#include <iomanip>

Snowflake::Snowflake()
    : m_num(Invalid) {}

Snowflake::Snowflake(uint64_t n)
    : m_num(n) {}

Snowflake::Snowflake(const std::string &str) {
    if (str.size())
        m_num = std::stoull(str);
    else
        m_num = Invalid;
}
Snowflake::Snowflake(const Glib::ustring &str) {
    if (str.size())
        m_num = std::strtoull(str.c_str(), nullptr, 10);
    else
        m_num = Invalid;
};

bool Snowflake::IsValid() const {
    return m_num != Invalid;
}

std::string Snowflake::GetLocalTimestamp() const {
    const time_t secs_since_epoch = (m_num / 4194304000) + 1420070400;
    const std::tm tm = *localtime(&secs_since_epoch);
    std::stringstream ss;
    const static std::locale locale("");
    ss.imbue(locale);
    ss << std::put_time(&tm, "%X %x");
    return ss.str();
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
