#pragma once
#include "websocket.hpp"
#include "http.hpp"
#include <nlohmann/json.hpp>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

// bruh
#ifdef GetMessage
#undef GetMessage
#endif

struct Snowflake {
    Snowflake();
    Snowflake(const Snowflake &s);
    Snowflake(uint64_t n);
    Snowflake(const std::string &str);

    bool IsValid() const;

    bool operator==(const Snowflake &s) const noexcept {
        return m_num == s.m_num;
    }

    bool operator<(const Snowflake &s) const noexcept {
        return m_num < s.m_num;
    }

    operator uint64_t() const noexcept {
        return m_num;
    }

    const static int Invalid = -1;

    friend void from_json(const nlohmann::json &j, Snowflake &s);
    friend void to_json(nlohmann::json &j, const Snowflake &s);

private:
    friend struct std::hash<Snowflake>;
    friend struct std::less<Snowflake>;
    unsigned long long m_num;
};

namespace std {
template<>
struct hash<Snowflake> {
    std::size_t operator()(const Snowflake &k) const {
        return k.m_num;
    }
};

template<>
struct less<Snowflake> {
    bool operator()(const Snowflake &l, const Snowflake &r) const {
        return l.m_num < r.m_num;
    }
};
} // namespace std

enum class GatewayOp : int {
    Event = 0,
    Heartbeat = 1,
    Identify = 2,
    Hello = 10,
    HeartbeatAck = 11,
};

enum class GatewayEvent : int {
    READY,
    MESSAGE_CREATE,
};

struct GatewayMessage {
    GatewayOp Opcode;
    nlohmann::json Data;
    std::string Type;

    friend void from_json(const nlohmann::json &j, GatewayMessage &m);
};

struct HelloMessageData {
    int HeartbeatInterval;

    friend void from_json(const nlohmann::json &j, HelloMessageData &m);
};

enum class ChannelType : int {
    GUILD_TEXT = 0,
    DM = 1,
    GUILD_VOICE = 2,
    GROUP_DM = 3,
    GUILD_CATEGORY = 4,
    GUILD_NEWS = 5,
    GUILD_STORE = 6,
};

struct UserData {
    Snowflake ID;              //
    std::string Username;      //
    std::string Discriminator; //
    std::string Avatar;        // null
    bool IsBot = false;        // opt
    bool IsSystem = false;     // opt
    bool IsMFAEnabled = false; // opt
    std::string Locale;        // opt
    bool IsVerified = false;   // opt
    std::string Email;         // opt, null
    int Flags = 0;             // opt
    int PremiumType = 0;       // opt
    int PublicFlags = 0;       // opt

    // undocumented (opt)
    bool IsDesktop = false;     //
    bool IsMobile = false;      //
    bool IsNSFWAllowed = false; // null
    std::string Phone;          // null?

    friend void from_json(const nlohmann::json &j, UserData &m);
};

struct ChannelData {
    Snowflake ID;      //
    ChannelType Type;  //
    Snowflake GuildID; // opt
    int Position = -1; // opt
    // std::vector<PermissionOverwriteData> PermissionOverwrites; // opt
    std::string Name;                 // opt
    std::string Topic;                // opt, null
    bool IsNSFW = false;              // opt
    Snowflake LastMessageID;          // opt, null
    int Bitrate = 0;                  // opt
    int UserLimit = 0;                // opt
    int RateLimitPerUser = 0;         // opt
    std::vector<UserData> Recipients; // opt
    std::string Icon;                 // opt, null
    Snowflake OwnerID;                // opt
    Snowflake ApplicationID;          // opt
    Snowflake ParentID;               // opt, null
    std::string LastPinTimestamp;     // opt, can be null even tho docs say otherwise

    friend void from_json(const nlohmann::json &j, ChannelData &m);
};

// a bot is apparently only supposed to receive the `id` and `unavailable` as false
// but user tokens seem to get the full objects (minus users)
struct GuildData {
    Snowflake ID;                    //
    std::string Name;                //
    std::string Icon;                // null
    std::string Splash;              // null
    std::string DiscoverySplash;     // null
    bool IsOwner = false;            // opt
    Snowflake OwnerID;               //
    int Permissions = 0;             // opt
    std::string PermissionsNew;      // opt
    std::string VoiceRegion;         // opt
    Snowflake AFKChannelID;          // null
    int AFKTimeout;                  //
    bool IsEmbedEnabled = false;     // opt, deprecated
    Snowflake EmbedChannelID;        // opt, null, deprecated
    int VerificationLevel;           //
    int DefaultMessageNotifications; //
    int ExplicitContentFilter;       //
    // std::vector<RoleData> Roles; //
    // std::vector<EmojiData> Emojis; //
    std::vector<std::string> Features; //
    int MFALevel;                      //
    Snowflake ApplicationID;           // null
    bool IsWidgetEnabled = false;      // opt
    Snowflake WidgetChannelID;         // opt, null
    Snowflake SystemChannelID;         // null
    int SystemChannelFlags;            //
    Snowflake RulesChannelID;          // null
    std::string JoinedAt;              // opt*
    bool IsLarge = false;              // opt*
    bool IsUnavailable = false;        // opt*
    int MemberCount = 0;               // opt*
    // std::vector<VoiceStateData> VoiceStates; // opt*
    // std::vector<MemberData> Members; // opt* - incomplete anyways
    std::vector<ChannelData> Channels; // opt*
    // std::vector<PresenceUpdateData> Presences; // opt*
    int MaxPresences = 0;             // opt, null
    int MaxMembers = 0;               // opt
    std::string VanityURL;            // null
    std::string Description;          // null
    std::string BannerHash;           // null
    int PremiumTier;                  //
    int PremiumSubscriptionCount = 0; // opt
    std::string PreferredLocale;      //
    Snowflake PublicUpdatesChannelID; // null
    int MaxVideoChannelUsers = 0;     // opt
    int ApproximateMemberCount = 0;   // opt
    int ApproximatePresenceCount = 0; // opt

    // undocumented
    // std::map<std::string, Unknown> GuildHashes;
    bool IsLazy = false;

    // * - documentation says only sent in GUILD_CREATE, but these can be sent anyways in the READY event

    friend void from_json(const nlohmann::json &j, GuildData &m);
};

struct UserSettingsData {
    int TimezoneOffset;                 //
    std::string Theme;                  //
    bool AreStreamNotificationsEnabled; //
    std::string Status;                 //
    bool ShouldShowCurrentGame;         //
    // std::vector<Unknown> RestrictedGuilds; //
    bool ShouldRenderReactions;            //
    bool ShouldRenderEmbeds;               //
    bool IsNativePhoneIntegrationEnabled;  //
    bool ShouldMessageDisplayCompact;      //
    std::string Locale;                    //
    bool ShouldInlineEmbedMedia;           //
    bool ShouldInlineAttachmentMedia;      //
    std::vector<Snowflake> GuildPositions; //
    // std::vector<GuildFolderEntryData> GuildFolders; //
    bool ShouldGIFAutoplay; //
    // Unknown FriendSourceFlags; //
    int ExplicitContentFilter;         //
    bool IsTTSCommandEnabled;          //
    bool ShouldDisableGamesTab;        //
    bool DeveloperMode;                //
    bool ShouldDetectPlatformAccounts; //
    bool AreDefaultGuildsRestricted;   //
    // Unknown CustomStatus; // null
    bool ShouldConvertEmoticons;          //
    bool IsContactSyncEnabled;            //
    bool ShouldAnimateEmojis;             //
    bool IsAccessibilityDetectionAllowed; //
    int AFKTimeout;

    friend void from_json(const nlohmann::json &j, UserSettingsData &m);
};

enum class MessageType {
    DEFAULT = 0,
    RECIPIENT_ADD = 1,
    RECIPIENT_REMOVE = 2,
    CALL = 3,
    CHANNEL_NaME_CHANGE = 4,
    CHANNEL_ICON_CHANGE = 5,
    CHANNEL_PINNED_MESSAGE = 6,
    GUILD_MEMBER_JOIN = 6,
    USER_PREMIUM_GUILD_SUBSCRIPTION = 7,
    USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_1 = 8,
    USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_2 = 9,
    USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_3 = 10,
    CHANNEL_FOLLOW_ADD = 12,
    GUILD_DISCOVERY_DISQUALIFIED = 13,
    GUILD_DISCOVERY_REQUALIFIED = 14,
};

enum class MessageFlags {
    NONE = 0,
    CROSSPOSTED = 1 << 0,
    IS_CROSSPOST = 1 << 1,
    SUPPRESS_EMBEDS = 1 << 2,
    SOURCE_MESSAGE_DELETE = 1 << 3,
    URGENT = 1 << 4,
};

struct MessageData {
    Snowflake ID;        //
    Snowflake ChannelID; //
    Snowflake GuildID;   // opt
    UserData Author;     //
    // GuildMemberData Member; // opt
    std::string Content;            //
    std::string Timestamp;          //
    std::string EditedTimestamp;    // null
    bool IsTTS;                     //
    bool DoesMentionEveryone;       //
    std::vector<UserData> Mentions; //
    // std::vector<RoleData> MentionRoles; //
    // std::vector<ChannelMentionData> MentionChannels; // opt
    // std::vector<AttachmentData> Attachments; //
    // std::vector<EmbedData> Embeds; //
    // std::vector<ReactionData> Reactions; // opt
    std::string Nonce;   // opt
    bool IsPinned;       //
    Snowflake WebhookID; // opt
    MessageType Type;    //
    // MessageActivityData Activity; // opt
    // MessageApplicationData Application; // opt
    // MessageReferenceData MessageReference; // opt
    MessageFlags Flags = MessageFlags::NONE; // opt

    friend void from_json(const nlohmann::json &j, MessageData &m);
};

struct ReadyEventData {
    int GatewayVersion;            //
    UserData User;                 //
    std::vector<GuildData> Guilds; //
    std::string SessionID;         //
    // std::vector<ChannelData?/PrivateChannelData?> PrivateChannels;

    // undocumented
    std::string AnalyticsToken;    // opt
    int FriendSuggestionCount;     // opt
    UserSettingsData UserSettings; // opt
    // std::vector<Unknown> ConnectedAccounts; // opt
    // std::map<std::string, Unknown> Consents; // opt
    // std::vector<Unknown> Experiments; // opt
    // std::vector<Unknown> GuildExperiments; // opt
    // std::map<Unknown, Unknown> Notes; // opt
    // std::vector<PresenceData> Presences; // opt
    // std::vector<ReadStateData> ReadStates; // opt
    // std::vector<RelationshipData> Relationships; // opt
    // Unknown Tutorial; // opt, null
    // std::vector<GuildSettingData> UserGuildSettings; // opt

    friend void from_json(const nlohmann::json &j, ReadyEventData &m);
};

struct IdentifyProperties {
    std::string OS;
    std::string Browser;
    std::string Device;

    friend void to_json(nlohmann::json &j, const IdentifyProperties &m);
};

struct IdentifyMessage : GatewayMessage {
    std::string Token;
    IdentifyProperties Properties;
    bool DoesSupportCompression = false;
    int LargeThreshold = 0;

    friend void to_json(nlohmann::json &j, const IdentifyMessage &m);
};

struct HeartbeatMessage : GatewayMessage {
    int Sequence;

    friend void to_json(nlohmann::json &j, const HeartbeatMessage &m);
};

// https://stackoverflow.com/questions/29775153/stopping-long-sleep-threads/29775639#29775639
class HeartbeatWaiter {
public:
    template<class R, class P>
    bool wait_for(std::chrono::duration<R, P> const &time) const {
        std::unique_lock<std::mutex> lock(m);
        return !cv.wait_for(lock, time, [&] { return terminate; });
    }
    void kill() {
        std::unique_lock<std::mutex> lock(m);
        terminate = true;
        cv.notify_all();
    }

private:
    mutable std::condition_variable cv;
    mutable std::mutex m;
    bool terminate = false;
};

class Abaddon;
class DiscordClient {
    friend class Abaddon;

public:
    static const constexpr char *DiscordGateway = "wss://gateway.discord.gg/?v=6&encoding=json";
    static const constexpr char *DiscordAPI = "https://discord.com/api";
    static const constexpr char *GatewayIdentity = "Discord";

public:
    DiscordClient();
    void SetAbaddon(Abaddon *ptr);
    void Start();
    void Stop();
    bool IsStarted() const;

    using Guilds_t = std::unordered_map<Snowflake, GuildData>;
    using Messages_t = std::unordered_map<Snowflake, MessageData>;

    const Guilds_t &GetGuilds() const;
    const UserSettingsData &GetUserSettings() const;
    std::vector<std::pair<Snowflake, GuildData>> GetUserSortedGuilds() const;
    std::unordered_set<const MessageData *> GetMessagesForChannel(Snowflake id) const;

    void UpdateSettingsGuildPositions(const std::vector<Snowflake> &pos);
    void FetchMessagesInChannel(Snowflake id, std::function<void(const std::vector<MessageData> &)> cb);
    const MessageData *GetMessage(Snowflake id) const;

    void UpdateToken(std::string token);

private:
    void HandleGatewayMessage(nlohmann::json msg);
    void HandleGatewayReady(const GatewayMessage &msg);
    void HandleGatewayMessageCreate(const GatewayMessage &msg);
    void HeartbeatThread();
    void SendIdentify();

    Abaddon *m_abaddon = nullptr;
    HTTPClient m_http;

    std::string m_token;

    mutable std::mutex m_mutex;

    void StoreGuild(Snowflake id, const GuildData &g);
    Guilds_t m_guilds;

    void StoreMessage(Snowflake id, const MessageData &m);
    Messages_t m_messages;
    std::unordered_map<Snowflake, std::unordered_set<const MessageData *>> m_chan_to_message_map;

    UserSettingsData m_user_settings;

    Websocket m_websocket;
    std::atomic<bool> m_client_connected = false;
    std::atomic<bool> m_ready_received = false;
    std::unordered_map<std::string, GatewayEvent> m_event_map;
    void LoadEventMap();

    std::thread m_heartbeat_thread;
    int m_last_sequence = -1;
    int m_heartbeat_msec = 0;
    HeartbeatWaiter m_heartbeat_waiter;
    bool m_heartbeat_acked = true;
};
