#include "store.hpp"

void Store::SetUser(Snowflake id, const User &user) {
    m_users[id] = user;
}

void Store::SetChannel(Snowflake id, const Channel &channel) {
    m_channels[id] = channel;
}

void Store::SetGuild(Snowflake id, const Guild &guild) {
    m_guilds[id] = guild;
}

void Store::SetRole(Snowflake id, const Role &role) {
    m_roles[id] = role;
}

void Store::SetMessage(Snowflake id, const Message &message) {
    m_messages[id] = message;
}

void Store::SetGuildMemberData(Snowflake guild_id, Snowflake user_id, const GuildMember &data) {
    m_members[guild_id][user_id] = data;
}

User *Store::GetUser(Snowflake id) {
    auto it = m_users.find(id);
    if (it == m_users.end())
        return nullptr;
    return &it->second;
}

const User *Store::GetUser(Snowflake id) const {
    auto it = m_users.find(id);
    if (it == m_users.end())
        return nullptr;
    return &it->second;
}

Channel *Store::GetChannel(Snowflake id) {
    auto it = m_channels.find(id);
    if (it == m_channels.end())
        return nullptr;
    return &it->second;
}

const Channel *Store::GetChannel(Snowflake id) const {
    auto it = m_channels.find(id);
    if (it == m_channels.end())
        return nullptr;
    return &it->second;
}

Guild *Store::GetGuild(Snowflake id) {
    auto it = m_guilds.find(id);
    if (it == m_guilds.end())
        return nullptr;
    return &it->second;
}

const Guild *Store::GetGuild(Snowflake id) const {
    auto it = m_guilds.find(id);
    if (it == m_guilds.end())
        return nullptr;
    return &it->second;
}

Role *Store::GetRole(Snowflake id) {
    auto it = m_roles.find(id);
    if (it == m_roles.end())
        return nullptr;
    return &it->second;
}

const Role *Store::GetRole(Snowflake id) const {
    auto it = m_roles.find(id);
    if (it == m_roles.end())
        return nullptr;
    return &it->second;
}

Message *Store::GetMessage(Snowflake id) {
    auto it = m_messages.find(id);
    if (it == m_messages.end())
        return nullptr;
    return &it->second;
}

const Message *Store::GetMessage(Snowflake id) const {
    auto it = m_messages.find(id);
    if (it == m_messages.end())
        return nullptr;
    return &it->second;
}

GuildMember *Store::GetGuildMemberData(Snowflake guild_id, Snowflake user_id) {
    auto git = m_members.find(guild_id);
    if (git == m_members.end())
        return nullptr;
    auto mit = git->second.find(user_id);
    if (mit == git->second.end())
        return nullptr;
    return &mit->second;
}

const GuildMember *Store::GetGuildMemberData(Snowflake guild_id, Snowflake user_id) const {
    auto git = m_members.find(guild_id);
    if (git == m_members.end())
        return nullptr;
    auto mit = git->second.find(user_id);
    if (mit == git->second.end())
        return nullptr;
    return &mit->second;
}

const Store::channels_type &Store::GetChannels() const {
    return m_channels;
}

const Store::guilds_type &Store::GetGuilds() const {
    return m_guilds;
}

void Store::ClearAll() {
    m_channels.clear();
    m_guilds.clear();
    m_messages.clear();
    m_roles.clear();
    m_users.clear();
}
