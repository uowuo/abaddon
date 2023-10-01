#pragma once
#include "objects.hpp"
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <filesystem>
#include <sqlite3.h>

#ifdef GetMessage // fuck you windows.h
#undef GetMessage
#endif

class Store {
private:
    class Statement;

public:
    Store(bool mem_store = false);
    ~Store();

    bool IsValid() const;

    void SetUser(Snowflake id, const UserData &user);
    void SetChannel(Snowflake id, const ChannelData &chan);
    void SetGuild(Snowflake id, const GuildData &guild);
    void SetRole(Snowflake guild_id, const RoleData &role);
    void SetMessage(Snowflake id, const Message &message);
    void SetGuildMember(Snowflake guild_id, Snowflake user_id, const GuildMember &data);
    void SetPermissionOverwrite(Snowflake channel_id, Snowflake id, const PermissionOverwrite &perm);
    void SetEmoji(Snowflake id, const EmojiData &emoji);
    void SetBan(Snowflake guild_id, Snowflake user_id, const BanData &ban);
    void SetWebhookMessage(const Message &message);

    std::optional<ChannelData> GetChannel(Snowflake id) const;
    std::optional<EmojiData> GetEmoji(Snowflake id) const;
    std::optional<GuildData> GetGuild(Snowflake id) const;
    std::optional<GuildMember> GetGuildMember(Snowflake guild_id, Snowflake user_id) const;
    std::optional<Message> GetMessage(Snowflake id) const;
    std::optional<PermissionOverwrite> GetPermissionOverwrite(Snowflake channel_id, Snowflake id) const;
    std::optional<RoleData> GetRole(Snowflake id) const;
    std::optional<UserData> GetUser(Snowflake id) const;
    std::optional<BanData> GetBan(Snowflake guild_id, Snowflake user_id) const;
    std::vector<BanData> GetBans(Snowflake guild_id) const;
    std::optional<WebhookMessageData> GetWebhookMessage(Snowflake message_id) const;

    Snowflake GetGuildOwner(Snowflake guild_id) const;
    std::vector<Snowflake> GetMemberRoles(Snowflake guild_id, Snowflake user_id) const;

    std::vector<Message> GetLastMessages(Snowflake id, size_t num) const;
    std::vector<Message> GetMessagesBefore(Snowflake channel_id, Snowflake message_id, size_t limit) const;
    std::vector<Message> GetPinnedMessages(Snowflake channel_id) const;
    std::vector<ChannelData> GetActiveThreads(Snowflake channel_id) const; // public
    std::vector<Snowflake> GetChannelIDsWithParentID(Snowflake channel_id) const;
    std::unordered_set<Snowflake> GetMembersInGuild(Snowflake guild_id) const;
    // ^ not the same as GetUsersInGuild since users in a guild may include users who do not have retrieved member data

    template<typename Iter>
    std::vector<UserData> GetUsersBulk(Iter begin, Iter end) {
        const int size = std::distance(begin, end);
        if (size == 0) return {};

        std::string query = "SELECT * FROM USERS WHERE id IN (";
        for (int i = 0; i < size; i++) {
            query += "?, ";
        }
        query.resize(query.size() - 2); // chop off extra ", "
        query += ")";

        Statement s(m_db, query.c_str());
        if (!s.OK()) {
            printf("failed to prepare bulk users: %s\n", m_db.ErrStr());
            return {};
        }

        for (int i = 0; begin != end; i++, begin++) {
            s.Bind(i, *begin);
        }

        std::vector<UserData> r;
        r.reserve(size);
        while (s.FetchOne()) {
            r.push_back(GetUserBound(&s));
        }
        return r;
    }

    void AddReaction(const MessageReactionAddObject &data, bool byself);
    void RemoveReaction(const MessageReactionRemoveObject &data, bool byself);

    void ClearGuild(Snowflake id);
    void ClearChannel(Snowflake id);
    void ClearBan(Snowflake guild_id, Snowflake user_id);
    void ClearRecipient(Snowflake channel_id, Snowflake user_id);
    void ClearRole(Snowflake id);

    std::unordered_set<Snowflake> GetChannels() const;
    std::unordered_set<Snowflake> GetGuilds() const;

    void ClearAll();

    void BeginTransaction();
    void EndTransaction();

private:
    class Database {
    public:
        Database(const char *path);
        ~Database();

        int Close();
        int StartTransaction();
        int EndTransaction();
        int Execute(const char *command);
        int Error() const;
        bool OK() const;
        const char *ErrStr() const;
        int SetError(int err);
        sqlite3 *obj();

    private:
        sqlite3 *m_db;
        int m_err = SQLITE_OK;
        mutable char m_err_scratch[256] { 0 };
        std::filesystem::path m_db_path;
    };

    class Statement {
    public:
        Statement() = delete;
        Statement(const Statement &other) = delete;
        Statement(Database &db, const char *command);
        ~Statement();
        Statement &operator=(Statement &other) = delete;

        [[nodiscard]] bool OK() const;

        int Bind(int index, Snowflake id);
        int Bind(int index, const char *str, size_t len = -1);
        int Bind(int index, const std::string &str);
        int Bind(int index);

        template<typename T>
        int Bind(int index, std::optional<T> opt) {
            if (opt.has_value())
                return Bind(index, opt.value());
            else
                return Bind(index);
        }

        template<typename Iter>
        int BindIDsAsJSON(int index, Iter start, Iter end) {
            std::vector<Snowflake> x;
            for (Iter it = start; it != end; it++) {
                x.push_back((*it).ID);
            }
            return Bind(index, nlohmann::json(x).dump());
        }

        template<typename T>
        int BindAsJSONArray(int index, const std::optional<T> &obj) {
            if (obj.has_value())
                return Bind(index, nlohmann::json(obj.value()).dump());
            else
                return Bind(index, std::string("[]"));
        }

        template<typename T>
        int BindAsJSON(int index, const T &obj) {
            return Bind(index, nlohmann::json(obj).dump());
        }

        template<typename T>
        inline typename std::enable_if<std::is_enum<T>::value, int>::type
        Bind(int index, T val) {
            return Bind(index, static_cast<typename std::underlying_type<T>::type>(val));
        }

        template<typename T>
        typename std::enable_if<std::is_integral<T>::value, int>::type
        Bind(int index, T val) {
            return m_db->SetError(sqlite3_bind_int64(m_stmt, index, val));
        }

        template<typename T>
        int BindAsJSON(int index, const std::optional<T> &obj) {
            if (obj.has_value())
                return Bind(index, nlohmann::json(obj.value()).dump());
            else
                return Bind(index);
        }

        template<typename T>
        typename std::enable_if<std::is_integral<T>::value>::type
        Get(int index, T &out) const {
            out = static_cast<T>(sqlite3_column_int64(m_stmt, index));
        }

        void Get(int index, Snowflake &out) const;
        void Get(int index, std::string &out) const;

        template<typename T>
        void GetJSON(int index, std::optional<T> &out) const {
            if (IsNull(index))
                out = std::nullopt;
            else {
                std::string stuff;
                Get(index, stuff);
                if (stuff == "")
                    out = std::nullopt;
                else
                    out = nlohmann::json::parse(stuff).get<T>();
            }
        }

        template<typename T>
        void GetJSON(int index, T &out) const {
            std::string stuff;
            Get(index, stuff);
            nlohmann::json::parse(stuff).get_to(out);
        }

        template<typename T>
        void Get(int index, std::optional<T> &out) const {
            if (IsNull(index))
                out = std::nullopt;
            else {
                T tmp;
                Get(index, tmp);
                out = std::optional<T>(std::move(tmp));
            }
        }

        template<typename T>
        inline typename std::enable_if<std::is_enum<T>::value, void>::type
        Get(int index, T &val) const {
            typename std::underlying_type<T>::type tmp;
            Get(index, tmp);
            val = static_cast<T>(tmp);
        }

        template<typename T>
        void GetIDOnlyStructs(int index, std::optional<std::vector<T>> &out) const {
            out.emplace();
            std::string str;
            Get(index, str);
            for (const auto &id : nlohmann::json::parse(str))
                out->emplace_back().ID = id.get<Snowflake>();
        }

        template<typename T, typename OutputIt>
        void GetArray(int index, OutputIt first) const {
            std::string str;
            Get(index, str);
            for (const auto &id : nlohmann::json::parse(str))
                *first++ = id.get<T>();
        }

        [[nodiscard]] bool IsNull(int index) const;
        int Step();
        bool Insert();
        bool FetchOne();
        int Reset();

        sqlite3_stmt *obj();

    private:
        Database *m_db;
        sqlite3_stmt *m_stmt;
    };

    UserData GetUserBound(Statement *stmt) const;
    Message GetMessageBound(std::unique_ptr<Statement> &stmt) const;
    static RoleData GetRoleBound(std::unique_ptr<Statement> &stmt);

    void SetMessageInteractionPair(Snowflake message_id, const MessageInteractionData &interaction);

    bool CreateTables();
    bool CreateStatements();

    bool m_ok = true;

    std::filesystem::path m_db_path;
    Database m_db;
#define STMT(x) mutable std::unique_ptr<Statement> m_stmt_##x
    STMT(set_guild);
    STMT(get_guild);
    STMT(get_guild_ids);
    STMT(clr_guild);
    STMT(set_chan);
    STMT(get_chan);
    STMT(get_chan_ids);
    STMT(clr_chan);
    STMT(set_msg);
    STMT(get_msg);
    STMT(set_msg_ref);
    STMT(get_last_msgs);
    STMT(set_user);
    STMT(get_user);
    STMT(set_member);
    STMT(get_member);
    STMT(set_role);
    STMT(get_role);
    STMT(get_guild_roles);
    STMT(set_emoji);
    STMT(get_emoji);
    STMT(set_perm);
    STMT(get_perm);
    STMT(set_ban);
    STMT(get_ban);
    STMT(get_bans);
    STMT(clr_ban);
    STMT(set_interaction);
    STMT(set_member_roles);
    STMT(get_member_roles);
    STMT(clr_member_roles);
    STMT(set_guild_emoji);
    STMT(get_guild_emojis);
    STMT(clr_guild_emoji);
    STMT(set_guild_feature);
    STMT(get_guild_features);
    STMT(get_guild_chans);
    STMT(set_thread);
    STMT(get_threads);
    STMT(get_active_threads);
    STMT(get_messages_before);
    STMT(get_pins);
    STMT(set_emoji_role);
    STMT(get_emoji_roles);
    STMT(set_mention);
    STMT(get_mentions);
    STMT(set_role_mention);
    STMT(get_role_mentions);
    STMT(set_attachment);
    STMT(get_attachments);
    STMT(set_recipient);
    STMT(get_recipients);
    STMT(clr_recipient);
    STMT(add_reaction);
    STMT(sub_reaction);
    STMT(get_reactions);
    STMT(get_chan_ids_parent);
    STMT(get_guild_member_ids);
    STMT(clr_role);
    STMT(get_guild_owner);
    STMT(set_webhook_msg);
    STMT(get_webhook_msg);
#undef STMT
};
