#include "snowflake.hpp"

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

void from_json(const nlohmann::json &j, Snowflake &s) {
    std::string tmp;
    j.get_to(tmp);
    s.m_num = std::stoull(tmp);
}

void to_json(nlohmann::json &j, const Snowflake &s) {
    j = std::to_string(s);
}
