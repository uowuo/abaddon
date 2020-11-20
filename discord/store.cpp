#include "store.hpp"

// hopefully the casting between signed and unsigned int64 doesnt cause issues

Store::Store() {
    m_db_path = std::filesystem::temp_directory_path() / "abaddon-store.db";
    m_db_err = sqlite3_open(m_db_path.string().c_str(), &m_db);
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

    std::filesystem::remove(m_db_path);
}

bool Store::IsValid() const {
    return m_db_err == SQLITE_OK;
}

void Store::SetUser(Snowflake id, const User &user) {
    if ((uint64_t)id == 0) {
        printf("???: %s\n", user.Username.c_str());
    }

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

    // m_users[id] = user;
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

void Store::SetPermissionOverwrite(Snowflake channel_id, Snowflake id, const PermissionOverwrite &perm) {
    m_permissions[channel_id][id] = perm;
}

void Store::SetEmoji(Snowflake id, const Emoji &emoji) {
    m_emojis[id] = emoji;
}

std::optional<User> Store::GetUser(Snowflake id) const {
    Bind(m_get_user_stmt, 1, id);
    if (!FetchOne(m_get_user_stmt)) {
        if (m_db_err != SQLITE_DONE) // not an error, just means user isnt found
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

    return std::optional<User>(std::move(ret));
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

PermissionOverwrite *Store::GetPermissionOverwrite(Snowflake channel_id, Snowflake id) {
    auto cit = m_permissions.find(channel_id);
    if (cit == m_permissions.end())
        return nullptr;
    auto pit = cit->second.find(id);
    if (pit == cit->second.end())
        return nullptr;
    return &pit->second;
}

Emoji *Store::GetEmoji(Snowflake id) {
    auto it = m_emojis.find(id);
    if (it != m_emojis.end())
        return &it->second;
    return nullptr;
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

const PermissionOverwrite *Store::GetPermissionOverwrite(Snowflake channel_id, Snowflake id) const {
    auto cit = m_permissions.find(channel_id);
    if (cit == m_permissions.end())
        return nullptr;
    auto pit = cit->second.find(id);
    if (pit == cit->second.end())
        return nullptr;
    return &pit->second;
}

const Emoji *Store::GetEmoji(Snowflake id) const {
    auto it = m_emojis.find(id);
    if (it != m_emojis.end())
        return &it->second;
    return nullptr;
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

const Store::roles_type &Store::GetRoles() const {
    return m_roles;
}

void Store::ClearAll() {
    m_channels.clear();
    m_emojis.clear();
    m_guilds.clear();
    m_members.clear();
    m_messages.clear();
    m_permissions.clear();
    m_roles.clear();
    m_users.clear();
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

    m_db_err = sqlite3_exec(m_db, create_users, nullptr, nullptr, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to create user table\n");
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

    m_db_err = sqlite3_prepare_v2(m_db, set_user, -1, &m_set_user_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare set user statement\n");
        return false;
    }

    m_db_err = sqlite3_prepare_v2(m_db, get_user, -1, &m_get_user_stmt, nullptr);
    if (m_db_err != SQLITE_OK) {
        fprintf(stderr, "failed to prepare get user statement\n");
        return false;
    }

    return true;
}

void Store::Cleanup() {
    sqlite3_finalize(m_set_user_stmt);
    sqlite3_finalize(m_get_user_stmt);
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
    m_db_err = sqlite3_bind_text(stmt, index, str.c_str(), -1, nullptr);
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

bool Store::RunInsert(sqlite3_stmt *stmt) {
    m_db_err = sqlite3_step(stmt);
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    return m_db_err == SQLITE_DONE;
}

bool Store::FetchOne(sqlite3_stmt *stmt) const {
    m_db_err = sqlite3_step(stmt);
    return m_db_err == SQLITE_ROW;
}

void Store::Get(sqlite3_stmt *stmt, int index, int &out) const {
    out = sqlite3_column_int(stmt, index);
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
