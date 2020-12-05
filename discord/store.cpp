#include "store.hpp"

using namespace std::literals::string_literals;

// hopefully the casting between signed and unsigned int64 doesnt cause issues

Store::Store(bool mem_store) {
    if (mem_store) {
        m_db_path = ":memory:";
        m_db_err = sqlite3_open(":memory:", &m_db);
    } else {
        m_db_path = std::filesystem::temp_directory_path() / "abaddon-store.db";
        m_db_err = sqlite3_open(m_db_path.string().c_str(), &m_db);
    }

    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "error opening database: %s\n", sqlite3_errstr(m_db_err));
        return;
    }

    m_db_err = sqlite3_exec(m_db, "PRAGMA journal_mode = WAL", nullptr, nullptr, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "enabling write-ahead-log failed: %s\n", sqlite3_errstr(m_db_err));
        return;
    }

    m_db_err = sqlite3_exec(m_db, "PRAGMA synchronous = NORMAL", nullptr, nullptr, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "setting synchronous failed: %s\n", sqlite3_errstr(m_db_err));
        return;
    }

    CreateTables();
    CreateStatements();
}

Store::~Store() {
    Cleanup();

    m_db_err = sqlite3_close(m_db);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "error closing database: %s\n", sqlite3_errstr(m_db_err));
        return;
    }

    if (m_db_path != ":memory:")
        std::filesystem::remove(m_db_path);
}

bool Store::IsValid() const {
    return m_db_err == SQLITE_OK;
}

void Store::SetUser(Snowflake id, const User &user) {
    Bind(m_set_user_stmt, 1, id);
    Bind(m_set_user_stmt, 2, user.Username);
    Bind(m_set_user_stmt, 3, user.Discriminator);
    Bind(m_set_user_stmt, 4, user.Avatar);
    Bind(m_set_user_stmt, 5, user.IsBot);
    Bind(m_set_user_stmt, 6, user.IsSystem);
    Bind(m_set_user_stmt, 7, user.IsMFAEnabled);
    Bind(m_set_user_stmt, 8, user.Locale);
    Bind(m_set_user_stmt, 9, user.IsVerified);
    Bind(m_set_user_stmt, 10, user.Email);
    Bind(m_set_user_stmt, 11, user.Flags);
    Bind(m_set_user_stmt, 12, user.PremiumType);
    Bind(m_set_user_stmt, 13, user.PublicFlags);

    if (!RunInsert(m_set_user_stmt)) {
        fprintf(stderr, "user insert failed: %s\n", sqlite3_errstr(m_db_err));
    }
}

void Store::SetChannel(Snowflake id, const Channel &channel) {
    m_channels[id] = channel;
}

void Store::SetGuild(Snowflake id, const Guild &guild) {
    m_guilds[id] = guild;
}

void Store::SetRole(Snowflake id, const Role &role) {
    Bind(m_set_role_stmt, 1, id);
    Bind(m_set_role_stmt, 2, role.Name);
    Bind(m_set_role_stmt, 3, role.Color);
    Bind(m_set_role_stmt, 4, role.IsHoisted);
    Bind(m_set_role_stmt, 5, role.Position);
    Bind(m_set_role_stmt, 6, static_cast<uint64_t>(role.Permissions));
    Bind(m_set_role_stmt, 7, role.IsManaged);
    Bind(m_set_role_stmt, 8, role.IsMentionable);

    if (!RunInsert(m_set_role_stmt))
        fprintf(stderr, "role insert failed: %s\n", sqlite3_errstr(m_db_err));
}

void Store::SetMessage(Snowflake id, const Message &message) {
    Bind(m_set_msg_stmt, 1, id);
    Bind(m_set_msg_stmt, 2, message.ChannelID);
    Bind(m_set_msg_stmt, 3, message.GuildID);
    Bind(m_set_msg_stmt, 4, message.Author.ID);
    Bind(m_set_msg_stmt, 5, message.Content);
    Bind(m_set_msg_stmt, 6, message.Timestamp);
    Bind(m_set_msg_stmt, 7, message.EditedTimestamp);
    Bind(m_set_msg_stmt, 8, message.IsTTS);
    Bind(m_set_msg_stmt, 9, message.DoesMentionEveryone);
    Bind(m_set_msg_stmt, 10, "[]"s); // mentions, a const char* literal will call the bool overload instead of std::string
    {
        std::string tmp;
        tmp = nlohmann::json(message.Attachments).dump();
        Bind(m_set_msg_stmt, 11, tmp);
    }
    {
        std::string tmp = nlohmann::json(message.Embeds).dump();
        Bind(m_set_msg_stmt, 12, tmp);
    }
    Bind(m_set_msg_stmt, 13, message.IsPinned);
    Bind(m_set_msg_stmt, 14, message.WebhookID);
    Bind(m_set_msg_stmt, 15, static_cast<uint64_t>(message.Type));

    if (message.MessageReference.has_value()) {
        std::string tmp = nlohmann::json(*message.MessageReference).dump();
        Bind(m_set_msg_stmt, 16, tmp);
    } else
        Bind(m_set_msg_stmt, 16, nullptr);

    if (message.Flags.has_value())
        Bind(m_set_msg_stmt, 17, static_cast<uint64_t>(*message.Flags));
    else
        Bind(m_set_msg_stmt, 17, nullptr);

    if (message.Stickers.has_value()) {
        std::string tmp = nlohmann::json(*message.Stickers).dump();
        Bind(m_set_msg_stmt, 18, tmp);
    } else
        Bind(m_set_msg_stmt, 18, nullptr);

    Bind(m_set_msg_stmt, 19, message.IsDeleted());
    Bind(m_set_msg_stmt, 20, message.IsEdited());

    if (!RunInsert(m_set_msg_stmt))
        fprintf(stderr, "message insert failed: %s\n", sqlite3_errstr(m_db_err));
}

void Store::SetGuildMember(Snowflake guild_id, Snowflake user_id, const GuildMember &data) {
    Bind(m_set_member_stmt, 1, user_id);
    Bind(m_set_member_stmt, 2, guild_id);
    Bind(m_set_member_stmt, 3, data.Nickname);
    Bind(m_set_member_stmt, 4, nlohmann::json(data.Roles).dump());
    Bind(m_set_member_stmt, 5, data.JoinedAt);
    Bind(m_set_member_stmt, 6, data.PremiumSince);
    Bind(m_set_member_stmt, 7, data.IsDeafened);
    Bind(m_set_member_stmt, 8, data.IsMuted);
    
    if (!RunInsert(m_set_member_stmt))
        fprintf(stderr, "member insert failed: %s\n", sqlite3_errstr(m_db_err));
}

void Store::SetPermissionOverwrite(Snowflake channel_id, Snowflake id, const PermissionOverwrite &perm) {
    Bind(m_set_perm_stmt, 1, perm.ID);
    Bind(m_set_perm_stmt, 2, channel_id);
    Bind(m_set_perm_stmt, 3, static_cast<int>(perm.Type));
    Bind(m_set_perm_stmt, 4, static_cast<uint64_t>(perm.Allow));
    Bind(m_set_perm_stmt, 5, static_cast<uint64_t>(perm.Deny));

    if (!RunInsert(m_set_perm_stmt))
        fprintf(stderr, "permission insert failed: %s\n", sqlite3_errstr(m_db_err));
}

void Store::SetEmoji(Snowflake id, const Emoji &emoji) {
    Bind(m_set_emote_stmt, 1, id);
    Bind(m_set_emote_stmt, 2, emoji.Name);

    if (emoji.Roles.has_value())
        Bind(m_set_emote_stmt, 3, nlohmann::json(*emoji.Roles).dump());
    else
        Bind(m_set_emote_stmt, 3, nullptr);

    if (emoji.Creator.has_value())
        Bind(m_set_emote_stmt, 4, emoji.Creator->ID);
    else
        Bind(m_set_emote_stmt, 4, nullptr);

    Bind(m_set_emote_stmt, 5, emoji.NeedsColons);
    Bind(m_set_emote_stmt, 6, emoji.IsManaged);
    Bind(m_set_emote_stmt, 7, emoji.IsAnimated);
    Bind(m_set_emote_stmt, 8, emoji.IsAvailable);

    if (!RunInsert(m_set_emote_stmt))
        fprintf(stderr, "emoji insert failed: %s\n", sqlite3_errstr(m_db_err));
}

std::optional<Emoji> Store::GetEmoji(Snowflake id) const {
    Bind(m_get_emote_stmt, 1, id);
    if (!FetchOne(m_get_emote_stmt)) {
        if (m_db_err != SQLITE_DONE)
            fprintf(stderr, "error while fetching emoji: %s\n", sqlite3_errstr(m_db_err));
        Reset(m_get_emote_stmt);
        return std::nullopt;
    }

    Emoji ret;
    ret.ID = id;
    Get(m_get_emote_stmt, 1, ret.Name);
    std::string tmp;
    Get(m_get_emote_stmt, 2, tmp);
    ret.Roles = nlohmann::json::parse(tmp).get<std::vector<Snowflake>>();
    ret.Creator = std::optional<User>(User());
    Get(m_get_emote_stmt, 3, ret.Creator->ID);
    Get(m_get_emote_stmt, 3, ret.NeedsColons);
    Get(m_get_emote_stmt, 4, ret.IsManaged);
    Get(m_get_emote_stmt, 5, ret.IsAnimated);
    Get(m_get_emote_stmt, 6, ret.IsAvailable);

    Reset(m_get_emote_stmt);

    return ret;
}

std::optional<GuildMember> Store::GetGuildMember(Snowflake guild_id, Snowflake user_id) const {
    Bind(m_get_member_stmt, 1, guild_id);
    Bind(m_get_member_stmt, 2, user_id);
    if (!FetchOne(m_get_member_stmt)) {
        if (m_db_err != SQLITE_DONE)
            fprintf(stderr, "error while fetching member: %s\n", sqlite3_errstr(m_db_err));
        Reset(m_get_member_stmt);
        return std::nullopt;
    }

    GuildMember ret;
    ret.User.emplace().ID = user_id;
    Get(m_get_member_stmt, 2, ret.Nickname);
    std::string tmp;
    Get(m_get_member_stmt, 3, tmp);
    ret.Roles = nlohmann::json::parse(tmp).get<std::vector<Snowflake>>();
    Get(m_get_member_stmt, 4, ret.JoinedAt);
    Get(m_get_member_stmt, 5, ret.PremiumSince);
    Get(m_get_member_stmt, 6, ret.IsDeafened);
    Get(m_get_member_stmt, 7, ret.IsMuted);

    Reset(m_get_member_stmt);

    return ret;
}

std::optional<Message> Store::GetMessage(Snowflake id) const {
    Bind(m_get_msg_stmt, 1, id);
    if (!FetchOne(m_get_msg_stmt)) {
        if (m_db_err != SQLITE_DONE)
            fprintf(stderr, "error while fetching message: %s\n", sqlite3_errstr(m_db_err));
        Reset(m_get_msg_stmt);
        return std::nullopt;
    }

    Message ret;
    ret.ID = id;
    Get(m_get_msg_stmt, 1, ret.ChannelID);
    Get(m_get_msg_stmt, 2, ret.GuildID);
    Get(m_get_msg_stmt, 3, ret.Author.ID); // yike
    Get(m_get_msg_stmt, 4, ret.Content);
    Get(m_get_msg_stmt, 5, ret.Timestamp);
    Get(m_get_msg_stmt, 6, ret.EditedTimestamp);
    Get(m_get_msg_stmt, 7, ret.IsTTS);
    Get(m_get_msg_stmt, 8, ret.DoesMentionEveryone);
    std::string tmps;
    Get(m_get_msg_stmt, 9, tmps);
    nlohmann::json::parse(tmps).get_to(ret.Mentions);
    Get(m_get_msg_stmt, 10, tmps);
    nlohmann::json::parse(tmps).get_to(ret.Attachments);
    Get(m_get_msg_stmt, 11, tmps);
    nlohmann::json::parse(tmps).get_to(ret.Embeds);
    Get(m_get_msg_stmt, 12, ret.IsPinned);
    Get(m_get_msg_stmt, 13, ret.WebhookID);
    uint64_t tmpi;
    Get(m_get_msg_stmt, 14, tmpi);
    ret.Type = static_cast<MessageType>(tmpi);
    Get(m_get_msg_stmt, 15, tmps);
    if (tmps != "")
        ret.MessageReference = nlohmann::json::parse(tmps).get<MessageReferenceData>();
    Get(m_get_msg_stmt, 16, tmpi);
    ret.Flags = static_cast<MessageFlags>(tmpi);
    Get(m_get_msg_stmt, 17, tmps);
    if (tmps != "")
        ret.Stickers = nlohmann::json::parse(tmps).get<std::vector<Sticker>>();
    bool tmpb = false;
    Get(m_get_msg_stmt, 18, tmpb);
    if (tmpb) ret.SetDeleted();
    Get(m_get_msg_stmt, 19, tmpb);
    if (tmpb) ret.SetEdited();

    Reset(m_get_msg_stmt);

    return std::optional<Message>(std::move(ret));
}

std::optional<PermissionOverwrite> Store::GetPermissionOverwrite(Snowflake channel_id, Snowflake id) const {
    Bind(m_get_perm_stmt, 1, id);
    Bind(m_get_perm_stmt, 2, channel_id);
    if (!FetchOne(m_get_perm_stmt)) {
        if (m_db_err != SQLITE_DONE)
            fprintf(stderr, "error while fetching permission: %s\n", sqlite3_errstr(m_db_err));
        Reset(m_get_perm_stmt);
        return std::nullopt;
    }

    PermissionOverwrite ret;
    ret.ID = id;
    uint64_t tmp;
    Get(m_get_perm_stmt, 2, tmp);
    ret.Type = static_cast<PermissionOverwrite::OverwriteType>(tmp);
    Get(m_get_perm_stmt, 3, tmp);
    ret.Allow = static_cast<Permission>(tmp);
    Get(m_get_perm_stmt, 4, tmp);
    ret.Deny = static_cast<Permission>(tmp);

    Reset(m_get_perm_stmt);

    return ret;
}

std::optional<Role> Store::GetRole(Snowflake id) const {
    Bind(m_get_role_stmt, 1, id);
    if (!FetchOne(m_get_role_stmt)) {
        if (m_db_err != SQLITE_DONE)
            fprintf(stderr, "error while fetching role: %s\n", sqlite3_errstr(m_db_err));
        Reset(m_get_role_stmt);
        return std::nullopt;
    }

    Role ret;
    ret.ID = id;
    Get(m_get_role_stmt, 1, ret.Name);
    Get(m_get_role_stmt, 2, ret.Color);
    Get(m_get_role_stmt, 3, ret.IsHoisted);
    Get(m_get_role_stmt, 4, ret.Position);
    uint64_t tmp;
    Get(m_get_role_stmt, 5, tmp);
    ret.Permissions = static_cast<Permission>(tmp);
    Get(m_get_role_stmt, 6, ret.IsManaged);
    Get(m_get_role_stmt, 7, ret.IsMentionable);

    Reset(m_get_role_stmt);

    return ret;
}

std::optional<User> Store::GetUser(Snowflake id) const {
    Bind(m_get_user_stmt, 1, id);
    if (!FetchOne(m_get_user_stmt)) {
        if (m_db_err != SQLITE_DONE)
            fprintf(stderr, "error while fetching user info: %s\n", sqlite3_errstr(m_db_err));
        Reset(m_get_user_stmt);
        return std::nullopt;
    }

    User ret;
    Get(m_get_user_stmt, 0, ret.ID);
    Get(m_get_user_stmt, 1, ret.Username);
    Get(m_get_user_stmt, 2, ret.Discriminator);
    Get(m_get_user_stmt, 3, ret.Avatar);
    Get(m_get_user_stmt, 4, ret.IsBot);
    Get(m_get_user_stmt, 5, ret.IsSystem);
    Get(m_get_user_stmt, 6, ret.IsMFAEnabled);
    Get(m_get_user_stmt, 7, ret.Locale);
    Get(m_get_user_stmt, 8, ret.IsVerified);
    Get(m_get_user_stmt, 9, ret.Email);
    Get(m_get_user_stmt, 10, ret.Flags);
    Get(m_get_user_stmt, 11, ret.PremiumType);
    Get(m_get_user_stmt, 12, ret.PublicFlags);

    Reset(m_get_user_stmt);

    return ret;
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

void Store::ClearGuild(Snowflake id) {
    m_guilds.erase(id);
}

void Store::ClearChannel(Snowflake id) {
    m_channels.erase(id);
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
    m_members.clear();
}

void Store::BeginTransaction() {
    m_db_err = sqlite3_exec(m_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
}

void Store::EndTransaction() {
    m_db_err = sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);
}

bool Store::CreateTables() {
    constexpr const char *create_users = R"(
CREATE TABLE IF NOT EXISTS users (
id INTEGER PRIMARY KEY,
username TEXT NOT NULL,
discriminator TEXT NOT NULL,
avatar TEXT,
bot BOOL,
system BOOL,
mfa BOOL,
locale TEXT,
verified BOOl,
email TEXT,
flags INTEGER,
premium INTEGER,
pubflags INTEGER
)
)";

    constexpr const char *create_permissions = R"(
CREATE TABLE IF NOT EXISTS permissions (
id INTEGER NOT NULL,
channel_id INTEGER NOT NULL,
type INTEGER NOT NULL,
allow INTEGER NOT NULL,
deny INTEGER NOT NULL
)
)";

    constexpr const char *create_messages = R"(
CREATE TABLE IF NOT EXISTS messages (
id INTEGER PRIMARY KEY,
channel_id INTEGER NOT NULL,
guild_id INTEGER,
author_id INTEGER NOT NULL,
content TEXT NOT NULL,
timestamp TEXT NOT NULL,
edited_timestamp TEXT,
tts BOOL NOT NULL,
everyone BOOL NOT NULL,
mentions TEXT NOT NULL, /* json */
attachments TEXT NOT NULL, /* json */
embeds TEXT NOT NULL, /* json */
pinned BOOL,
webhook_id INTEGER,
type INTEGER,
reference TEXT, /* json */
flags INTEGER,
stickers TEXT, /* json */
/* extra */
deleted BOOL,
edited BOOL
)
)";

    constexpr const char *create_roles = R"(
CREATE TABLE IF NOT EXISTS roles (
id INTEGER PRIMARY KEY,
name TEXT NOT NULL,
color INTEGER NOT NULL,
hoisted BOOL NOT NULL,
position INTEGER NOT NULL,
permissions INTEGER NOT NULL,
managed BOOL NOT NULL,
mentionable BOOL NOT NULL
)
)";

    constexpr const char *create_emojis = R"(
CREATE TABLE IF NOT EXISTS emojis (
id INTEGER PRIMARY KEY, /*though nullable, only custom emojis (with non-null ids) are stored*/
name TEXT NOT NULL, /*same as id*/
roles TEXT, /* json */
creator_id INTEGER,
colons BOOL,
managed BOOL,
animated BOOL,
available BOOL
)
)";

    constexpr const char *create_members = R"(
CREATE TABLE IF NOT EXISTS members (
user_id INTEGER PRIMARY KEY,
guild_id INTEGER NOT NULL,
nickname TEXT,
roles TEXT NOT NULL, /* json */
joined_at TEXT NOT NULL,
premium_since TEXT,
deaf BOOL NOT NULL,
mute BOOL NOT NULL
)
)";

    m_db_err = sqlite3_exec(m_db, create_users, nullptr, nullptr, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to create user table: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_exec(m_db, create_permissions, nullptr, nullptr, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to create permissions table: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_exec(m_db, create_messages, nullptr, nullptr, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to create messages table: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_exec(m_db, create_roles, nullptr, nullptr, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to create roles table: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_exec(m_db, create_emojis, nullptr, nullptr, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "faile to create emojis table: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_exec(m_db, create_members, nullptr, nullptr, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to create members table: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    return true;
}

bool Store::CreateStatements() {
    constexpr const char *set_user = R"(
REPLACE INTO users VALUES (
    ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
)
)";

    constexpr const char *get_user = R"(
SELECT * FROM users WHERE id = ?
)";

    constexpr const char *set_perm = R"(
REPLACE INTO permissions VALUES (
    ?, ?, ?, ?, ?
)
)";

    constexpr const char *get_perm = R"(
SELECT * FROM permissions WHERE id = ? AND channel_id = ?
)";

    constexpr const char *set_msg = R"(
REPLACE INTO messages VALUES (
    ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
)
)";

    constexpr const char *get_msg = R"(
SELECT * FROM messages WHERE id = ?
)";

    constexpr const char *set_role = R"(
REPLACE INTO roles VALUES (
    ?, ?, ?, ?, ?, ?, ?, ?
)
)";

    constexpr const char *get_role = R"(
SELECT * FROM roles WHERE id = ?
)";

    constexpr const char *set_emoji = R"(
REPLACE INTO emojis VALUES (
    ?, ?, ?, ?, ?, ?, ?, ?
)
)";

    constexpr const char *get_emoji = R"(
SELECT * FROM emojis WHERE id = ?
)";

    constexpr const char *set_member = R"(
REPLACE INTO members VALUES (
    ?, ?, ?, ?, ?, ?, ?, ?
)
)";

    constexpr const char *get_member = R"(
SELECT * FROM members WHERE user_id = ? AND guild_id = ?
)";

    m_db_err = sqlite3_prepare_v2(m_db, set_user, -1, &m_set_user_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare set user statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, get_user, -1, &m_get_user_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare get user statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, set_perm, -1, &m_set_perm_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare set permission statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, get_perm, -1, &m_get_perm_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare get permission statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, set_msg, -1, &m_set_msg_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare set message statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, get_msg, -1, &m_get_msg_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare get message statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, set_role, -1, &m_set_role_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare set role statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, get_role, -1, &m_get_role_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare get role statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, set_emoji, -1, &m_set_emote_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare set emoji statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, get_emoji, -1, &m_get_emote_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare get emoji statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, set_member, -1, &m_set_member_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare set member statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, get_member, -1, &m_get_member_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare get member statement: %s\n", sqlite3_errstr(m_db_err));
        return false;
    }

    return true;
}

void Store::Cleanup() {
    sqlite3_finalize(m_set_user_stmt);
    sqlite3_finalize(m_get_user_stmt);
    sqlite3_finalize(m_set_perm_stmt);
    sqlite3_finalize(m_get_perm_stmt);
    sqlite3_finalize(m_set_msg_stmt);
    sqlite3_finalize(m_get_msg_stmt);
    sqlite3_finalize(m_set_role_stmt);
    sqlite3_finalize(m_get_role_stmt);
    sqlite3_finalize(m_set_emote_stmt);
    sqlite3_finalize(m_get_emote_stmt);
    sqlite3_finalize(m_set_member_stmt);
    sqlite3_finalize(m_get_member_stmt);
}

void Store::Bind(sqlite3_stmt *stmt, int index, int num) const {
    m_db_err = sqlite3_bind_int(stmt, index, num);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "error binding index %d: %s\n", index, sqlite3_errstr(m_db_err));
    }
}

void Store::Bind(sqlite3_stmt *stmt, int index, uint64_t num) const {
    m_db_err = sqlite3_bind_int64(stmt, index, num);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "error binding index %d: %s\n", index, sqlite3_errstr(m_db_err));
    }
}

void Store::Bind(sqlite3_stmt *stmt, int index, const std::string &str) const {
    m_db_err = sqlite3_bind_blob(stmt, index, str.c_str(), str.length(), SQLITE_TRANSIENT);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "error binding index %d: %s\n", index, sqlite3_errstr(m_db_err));
    }
}

void Store::Bind(sqlite3_stmt *stmt, int index, bool val) const {
    m_db_err = sqlite3_bind_int(stmt, index, val ? 1 : 0);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "error binding index %d: %s\n", index, sqlite3_errstr(m_db_err));
    }
}

void Store::Bind(sqlite3_stmt *stmt, int index, std::nullptr_t) const {
    m_db_err = sqlite3_bind_null(stmt, index);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "error binding index %d: %s\n", index, sqlite3_errstr(m_db_err));
    }
}

bool Store::RunInsert(sqlite3_stmt *stmt) {
    m_db_err = sqlite3_step(stmt);
    Reset(stmt);
    return m_db_err == SQLITE_DONE;
}

bool Store::FetchOne(sqlite3_stmt *stmt) const {
    m_db_err = sqlite3_step(stmt);
    return m_db_err == SQLITE_ROW;
}

void Store::Get(sqlite3_stmt *stmt, int index, int &out) const {
    out = sqlite3_column_int(stmt, index);
}

void Store::Get(sqlite3_stmt *stmt, int index, uint64_t &out) const {
    out = sqlite3_column_int64(stmt, index);
}

void Store::Get(sqlite3_stmt *stmt, int index, std::string &out) const {
    const unsigned char *ptr = sqlite3_column_text(stmt, index);
    if (ptr == nullptr)
        out = "";
    else
        out = reinterpret_cast<const char *>(ptr);
}

void Store::Get(sqlite3_stmt *stmt, int index, bool &out) const {
    out = sqlite3_column_int(stmt, index) != 0;
}

void Store::Get(sqlite3_stmt *stmt, int index, Snowflake &out) const {
    const int64_t num = sqlite3_column_int64(stmt, index);
    out = static_cast<uint64_t>(num);
}

void Store::Reset(sqlite3_stmt *stmt) const {
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
}
