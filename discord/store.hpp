#pragma once
#include "objects.hpp"
#include <unordered_map>
#include <mutex>

#ifdef GetMessage // fuck you windows.h
    #undef GetMessage
#endif

class Store {
public:
    void SetUser(Snowflake id, const User &user);
    void SetChannel(Snowflake id, const Channel &channel);
    void SetGuild(Snowflake id, const Guild &guild);
    void SetRole(Snowflake id, const Role &role);
    void SetMessage(Snowflake id, const Message &message);
    void SetGuildMemberData(Snowflake guild_id, Snowflake user_id, const GuildMember &data);

    User *GetUser(Snowflake id);
    Channel *GetChannel(Snowflake id);
    Guild *GetGuild(Snowflake id);
    Role *GetRole(Snowflake id);
    Message *GetMessage(Snowflake id);
    GuildMember *GetGuildMemberData(Snowflake guild_id, Snowflake user_id);
    const User *GetUser(Snowflake id) const;
    const Channel *GetChannel(Snowflake id) const;
    const Guild *GetGuild(Snowflake id) const;
    const Role *GetRole(Snowflake id) const;
    const Message *GetMessage(Snowflake id) const;
    const GuildMember *GetGuildMemberData(Snowflake guild_id, Snowflake user_id) const;

    void ClearGuild(Snowflake id);
    void ClearChannel(Snowflake id);

    using users_type = std::unordered_map<Snowflake, User>;
    using channels_type = std::unordered_map<Snowflake, Channel>;
    using guilds_type = std::unordered_map<Snowflake, Guild>;
    using roles_type = std::unordered_map<Snowflake, Role>;
    using messages_type = std::unordered_map<Snowflake, Message>;
    using members_type = std::unordered_map<Snowflake, std::unordered_map<Snowflake, GuildMember>>;

    const channels_type &GetChannels() const;
    const guilds_type &GetGuilds() const;

    void ClearAll();

private:
    users_type m_users;
    channels_type m_channels;
    guilds_type m_guilds;
    roles_type m_roles;
    messages_type m_messages;
    members_type m_members;
};
