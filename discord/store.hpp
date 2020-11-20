#pragma once
#include "../util.hpp"
#include "objects.hpp"
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include <sqlite3.h>

#ifdef GetMessage // fuck you windows.h
    #undef GetMessage
#endif

class Store {
public:
    Store();
    ~Store();

    bool IsValid() const;

    void SetUser(Snowflake id, const User &user);
    void SetChannel(Snowflake id, const Channel &channel);
    void SetGuild(Snowflake id, const Guild &guild);
    void SetRole(Snowflake id, const Role &role);
    void SetMessage(Snowflake id, const Message &message);
    void SetGuildMemberData(Snowflake guild_id, Snowflake user_id, const GuildMember &data);
    void SetPermissionOverwrite(Snowflake channel_id, Snowflake id, const PermissionOverwrite &perm);
    void SetEmoji(Snowflake id, const Emoji &emoji);

    // slap const on everything even tho its not *really* const

    Channel *GetChannel(Snowflake id);
    Guild *GetGuild(Snowflake id);
    Role *GetRole(Snowflake id);
    Message *GetMessage(Snowflake id);
    GuildMember *GetGuildMemberData(Snowflake guild_id, Snowflake user_id);
    PermissionOverwrite *GetPermissionOverwrite(Snowflake channel_id, Snowflake id);
    Emoji *GetEmoji(Snowflake id);
    std::optional<User> GetUser(Snowflake id) const;
    const Channel *GetChannel(Snowflake id) const;
    const Guild *GetGuild(Snowflake id) const;
    const Role *GetRole(Snowflake id) const;
    const Message *GetMessage(Snowflake id) const;
    const GuildMember *GetGuildMemberData(Snowflake guild_id, Snowflake user_id) const;
    const PermissionOverwrite *GetPermissionOverwrite(Snowflake channel_id, Snowflake id) const;
    const Emoji *GetEmoji(Snowflake id) const;

    void ClearGuild(Snowflake id);
    void ClearChannel(Snowflake id);

    using users_type = std::unordered_map<Snowflake, User>;
    using channels_type = std::unordered_map<Snowflake, Channel>;
    using guilds_type = std::unordered_map<Snowflake, Guild>;
    using roles_type = std::unordered_map<Snowflake, Role>;
    using messages_type = std::unordered_map<Snowflake, Message>;
    using members_type = std::unordered_map<Snowflake, std::unordered_map<Snowflake, GuildMember>>;                       // [guild][user]
    using permission_overwrites_type = std::unordered_map<Snowflake, std::unordered_map<Snowflake, PermissionOverwrite>>; // [channel][user/role]
    using emojis_type = std::unordered_map<Snowflake, Emoji>;

    const channels_type &GetChannels() const;
    const guilds_type &GetGuilds() const;
    const roles_type &GetRoles() const;

    void ClearAll();

    void BeginTransaction();
    void EndTransaction();

private:
    users_type m_users;
    channels_type m_channels;
    guilds_type m_guilds;
    roles_type m_roles;
    messages_type m_messages;
    members_type m_members;
    permission_overwrites_type m_permissions;
    emojis_type m_emojis;

    bool CreateTables();
    bool CreateStatements();
    void Cleanup();

    template<typename T>
    void Bind(sqlite3_stmt *stmt, int index, const std::optional<T> &opt) const;
    void Bind(sqlite3_stmt *stmt, int index, int num) const;
    void Bind(sqlite3_stmt *stmt, int index, uint64_t num) const;
    void Bind(sqlite3_stmt *stmt, int index, const std::string &str) const;
    void Bind(sqlite3_stmt *stmt, int index, bool val) const;
    bool RunInsert(sqlite3_stmt *stmt);
    bool FetchOne(sqlite3_stmt *stmt) const;
    template<typename T>
    void Get(sqlite3_stmt *stmt, int index, std::optional<T> &out) const;
    void Get(sqlite3_stmt *stmt, int index, int &out) const;
    void Get(sqlite3_stmt *stmt, int index, uint64_t &out) const;
    void Get(sqlite3_stmt *stmt, int index, std::string &out) const;
    void Get(sqlite3_stmt *stmt, int index, bool &out) const;
    void Get(sqlite3_stmt *stmt, int index, Snowflake &out) const;
    void Reset(sqlite3_stmt *stmt) const;

    std::filesystem::path m_db_path;
    mutable sqlite3 *m_db;
    mutable int m_db_err;
    mutable sqlite3_stmt *m_set_user_stmt;
    mutable sqlite3_stmt *m_get_user_stmt;
};

template<typename T>
inline void Store::Bind(sqlite3_stmt *stmt, int index, const std::optional<T> &opt) const {
    if (opt.has_value())
        Bind(stmt, index, *opt);
    else
        sqlite3_bind_null(stmt, index);
}

template<typename T>
inline void Store::Get(sqlite3_stmt *stmt, int index, std::optional<T> &out) const {
    if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
        out = std::nullopt;
    else {
        T v;
        Get(stmt, index, v);
        out = std::optional<T>(v);
    }
}
