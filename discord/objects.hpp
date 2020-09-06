#pragma once
#include <algorithm>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

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
    LazyLoadRequest = 14,
};

enum class GatewayEvent : int {
    READY,
    MESSAGE_CREATE,
    MESSAGE_DELETE,
    MESSAGE_UPDATE,
    GUILD_MEMBER_LIST_UPDATE,
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

struct RoleData {
    Snowflake ID;
    std::string Name;
    int Color;
    bool IsHoisted;
    int Position;
    int PermissionsLegacy;
    uint64_t Permissions;
    bool IsManaged;
    bool IsMentionable;

    friend void from_json(const nlohmann::json &j, RoleData &m);
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
    int PremiumType = 0;       // opt, null (docs wrong)
    int PublicFlags = 0;       // opt

    // undocumented (opt)
    bool IsDesktop = false;     //
    bool IsMobile = false;      //
    bool IsNSFWAllowed = false; // null
    std::string Phone;          // null?

    friend void from_json(const nlohmann::json &j, UserData &m);
};

struct GuildMemberData {
    UserData User;                // opt
    std::string Nickname;         // null
    std::vector<Snowflake> Roles; //
    std::string JoinedAt;         //
    std::string PremiumSince;     // opt, null
    bool IsDeafened;              //
    bool IsMuted;                 //

    friend void from_json(const nlohmann::json &j, GuildMemberData &m);
};

struct ChannelData {
    Snowflake ID;      //
    ChannelType Type;  //
    Snowflake GuildID; // opt
    int Position = -1; // opt
    // std::vector<PermissionOverwriteData> PermissionOverwrites; // opt
    std::string Name;                 // opt, null (null for dm's)
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
    std::string DiscoverySplash;     // opt, null (docs wrong)
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
    std::vector<RoleData> Roles;     //
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

struct EmbedFooterData {
    std::string Text;         //
    std::string IconURL;      // opt
    std::string ProxyIconURL; // opt

    friend void from_json(const nlohmann::json &j, EmbedFooterData &m);
};

struct EmbedImageData {
    std::string URL;      // opt
    std::string ProxyURL; // opt
    int Height = 0;       // opt
    int Width = 0;        // opt

    friend void from_json(const nlohmann::json &j, EmbedImageData &m);
};

struct EmbedThumbnailData {
    std::string URL;      // opt
    std::string ProxyURL; // opt
    int Height = 0;       // opt
    int Width = 0;        // opt

    friend void from_json(const nlohmann::json &j, EmbedThumbnailData &m);
};

struct EmbedVideoData {
    std::string URL; // opt
    int Height = 0;  // opt
    int Width = 0;   // opt
    friend void from_json(const nlohmann::json &j, EmbedVideoData &m);
};

struct EmbedProviderData {
    std::string Name; // opt
    std::string URL;  // opt, null (docs wrong)

    friend void from_json(const nlohmann::json &j, EmbedProviderData &m);
};

struct EmbedAuthorData {
    std::string Name;         // opt
    std::string URL;          // opt
    std::string IconURL;      // opt
    std::string ProxyIconURL; // opt

    friend void from_json(const nlohmann::json &j, EmbedAuthorData &m);
};

struct EmbedFieldData {
    std::string Name;    //
    std::string Value;   //
    bool Inline = false; // opt

    friend void from_json(const nlohmann::json &j, EmbedFieldData &m);
};

struct EmbedData {
    std::string Title;                  // opt
    std::string Type;                   // opt
    std::string Description;            // opt
    std::string URL;                    // opt
    std::string Timestamp;              // opt
    int Color = -1;                     // opt
    EmbedFooterData Footer;             // opt
    EmbedImageData Image;               // opt
    EmbedThumbnailData Thumbnail;       // opt
    EmbedVideoData Video;               // opt
    EmbedProviderData Provider;         // opt
    EmbedAuthorData Author;             // opt
    std::vector<EmbedFieldData> Fields; // opt

    friend void from_json(const nlohmann::json &j, EmbedData &m);
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
    std::vector<EmbedData> Embeds; //
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
    void from_json_edited(const nlohmann::json &j); // for MESSAGE_UPDATE
};

struct MessageDeleteData {
    Snowflake ID;        //
    Snowflake ChannelID; //
    Snowflake GuildID;   // opt

    friend void from_json(const nlohmann::json &j, MessageDeleteData &m);
};

struct GuildMemberListUpdateMessage {
    struct Item {
        std::string Type;
    };

    struct GroupItem : Item {
        std::string ID;
        int Count;

        friend void from_json(const nlohmann::json &j, GroupItem &m);
    };

    struct MemberItem : Item {
        UserData User;                //
        std::vector<Snowflake> Roles; //
        // PresenceData Presence; //
        std::string PremiumSince; // opt
        std::string Nickname;     // opt
        bool IsMuted;             //
        std::string JoinedAt;     //
        std::string HoistedRole;  // null
        bool IsDefeaned;          //

        GuildMemberData GetAsMemberData() const;

        friend void from_json(const nlohmann::json &j, MemberItem &m);

    private:
        GuildMemberData m_member_data;
    };

    struct OpObject {
        std::string Op;
        int Index;
        std::vector<std::unique_ptr<Item>> Items; // SYNC
        std::pair<int, int> Range;                // SYNC

        friend void from_json(const nlohmann::json &j, OpObject &m);
    };

    int OnlineCount;
    int MemberCount;
    std::string ListIDHash;
    std::string GuildID;
    std::vector<GroupItem> Groups;
    std::vector<OpObject> Ops;

    friend void from_json(const nlohmann::json &j, GuildMemberListUpdateMessage &m);
};

struct LazyLoadRequestMessage {
    Snowflake GuildID;
    bool ShouldGetTyping = false;
    bool ShouldGetActivities = false;
    std::vector<std::string> Members;                                         // snowflake?
    std::unordered_map<Snowflake, std::vector<std::pair<int, int>>> Channels; // channel ID -> range of sidebar

    friend void to_json(nlohmann::json &j, const LazyLoadRequestMessage &m);
};

struct ReadyEventData {
    int GatewayVersion;                       //
    UserData User;                            //
    std::vector<GuildData> Guilds;            //
    std::string SessionID;                    //
    std::vector<ChannelData> PrivateChannels; //

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

struct CreateMessageObject {
    std::string Content;

    friend void to_json(nlohmann::json &j, const CreateMessageObject &m);
};

struct MessageEditObject {
    std::string Content;           // opt, null
    std::vector<EmbedData> Embeds; // opt, null
    int Flags = -1;                // opt, null

    friend void to_json(nlohmann::json &j, const MessageEditObject &m);
};
