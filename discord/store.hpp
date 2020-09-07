#pragma once
#include "objects.hpp"
#include <unordered_map>
#include <mutex>

#ifdef GetMessage // fuck you windows.h
    #undef GetMessage
#endif

class Store {
public:
    void SetUser(Snowflake id, const UserData &user);
    void SetChannel(Snowflake id, const ChannelData &channel);
    void SetGuild(Snowflake id, const GuildData &guild);
    void SetRole(Snowflake id, const RoleData &role);
    void SetMessage(Snowflake id, const MessageData &message);
    void SetGuildMemberData(Snowflake guild_id, Snowflake user_id, const GuildMemberData &data);

    const UserData *GetUser(Snowflake id) const;
    const ChannelData *GetChannel(Snowflake id) const;
    const GuildData *GetGuild(Snowflake id) const;
    const RoleData *GetRole(Snowflake id) const;
    const MessageData *GetMessage(Snowflake id) const;
    const GuildMemberData *GetGuildMemberData(Snowflake guild_id, Snowflake user_id) const;

    using users_type = std::unordered_map<Snowflake, UserData>;
    using channels_type = std::unordered_map<Snowflake, ChannelData>;
    using guilds_type = std::unordered_map<Snowflake, GuildData>;
    using roles_type = std::unordered_map<Snowflake, RoleData>;
    using messages_type = std::unordered_map<Snowflake, MessageData>;
    using members_type = std::unordered_map<Snowflake, std::unordered_map<Snowflake, GuildMemberData>>;

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
