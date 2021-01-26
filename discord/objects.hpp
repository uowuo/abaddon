#pragma once
#include <algorithm>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include "snowflake.hpp"
#include "user.hpp"
#include "role.hpp"
#include "member.hpp"
#include "channel.hpp"
#include "guild.hpp"
#include "usersettings.hpp"
#include "message.hpp"
#include "invite.hpp"
#include "permissions.hpp"
#include "emoji.hpp"
#include "activity.hpp"
#include "sticker.hpp"
#include "ban.hpp"
#include "auditlog.hpp"

// most stuff below should just be objects that get processed and thrown away immediately

enum class GatewayOp : int {
    Event = 0,
    Heartbeat = 1,
    Identify = 2,
    UpdateStatus = 3,
    Resume = 6,
    Reconnect = 7,
    InvalidSession = 9,
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
    GUILD_CREATE,
    GUILD_DELETE,
    MESSAGE_DELETE_BULK,
    GUILD_MEMBER_UPDATE,
    PRESENCE_UPDATE,
    CHANNEL_DELETE,
    CHANNEL_UPDATE,
    CHANNEL_CREATE,
    GUILD_UPDATE,
    GUILD_ROLE_UPDATE,
    GUILD_ROLE_CREATE,
    GUILD_ROLE_DELETE,
    MESSAGE_REACTION_ADD,
    MESSAGE_REACTION_REMOVE,
    CHANNEL_RECIPIENT_ADD,
    CHANNEL_RECIPIENT_REMOVE,
    TYPING_START,
    GUILD_BAN_REMOVE,
    GUILD_BAN_ADD,
    INVITE_CREATE,
    INVITE_DELETE,
};

enum class GatewayCloseCode : uint16_t {
    // discord
    UnknownError = 4000,
    UnknownOpcode = 4001,
    DecodeError = 4002,
    NotAuthenticated = 4003,
    AuthenticationFailed = 4004,
    AlreadyAuthenticated = 4005,
    InvalidSequence = 4007,
    RateLimited = 4008,
    SessionTimedOut = 4009,
    InvalidShard = 4010,
    ShardingRequired = 4011,
    InvalidAPIVersion = 4012,
    InvalidIntents = 4013,
    DisallowedIntents = 4014,

    // internal
    UserDisconnect = 4091,
    Reconnecting = 4092,
};

struct GatewayMessage {
    GatewayOp Opcode;
    nlohmann::json Data;
    std::string Type;
    int Sequence = -1;

    friend void from_json(const nlohmann::json &j, GatewayMessage &m);
};

struct HelloMessageData {
    int HeartbeatInterval;

    friend void from_json(const nlohmann::json &j, HelloMessageData &m);
};

struct MessageDeleteData {
    Snowflake ID;        //
    Snowflake ChannelID; //
    Snowflake GuildID;   // opt

    friend void from_json(const nlohmann::json &j, MessageDeleteData &m);
};

struct MessageDeleteBulkData {
    std::vector<Snowflake> IDs; //
    Snowflake ChannelID;        //
    Snowflake GuildID;          // opt

    friend void from_json(const nlohmann::json &j, MessageDeleteBulkData &m);
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
        UserData User;
        std::vector<Snowflake> Roles;
        std::optional<PresenceData> Presence;
        std::string PremiumSince; // opt
        std::string Nickname;     // opt
        bool IsMuted;
        std::string JoinedAt;
        std::string HoistedRole; // null
        bool IsDefeaned;

        GuildMember GetAsMemberData() const;

        friend void from_json(const nlohmann::json &j, MemberItem &m);

    private:
        GuildMember m_member_data;
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

struct UpdateStatusMessage {
    int Since = 0;
    std::vector<ActivityData> Activities;
    PresenceStatus Status;
    bool IsAFK = false;

    friend void to_json(nlohmann::json &j, const UpdateStatusMessage &m);
};

struct ReadyEventData {
    int GatewayVersion;
    UserData SelfUser;
    std::vector<GuildData> Guilds;
    std::string SessionID;
    std::vector<ChannelData> PrivateChannels;

    // undocumented
    std::optional<std::vector<UserData>> Users;
    std::optional<std::string> AnalyticsToken;
    std::optional<int> FriendSuggestionCount;
    UserSettings Settings;
    std::optional<std::vector<std::vector<GuildMember>>> MergedMembers;
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
    std::string BrowserUserAgent;
    std::string BrowserVersion;
    std::string OSVersion;
    std::string Referrer;
    std::string ReferringDomain;
    std::string ReferrerCurrent;
    std::string ReferringDomainCurrent;
    std::string ReleaseChannel;
    int ClientBuildNumber;
    std::string ClientEventSource; // empty -> null

    friend void to_json(nlohmann::json &j, const IdentifyProperties &m);
};

struct ClientStateProperties {
    std::map<std::string, std::string> GuildHashes;
    std::string HighestLastMessageID = "0";
    int ReadStateVersion = 0;
    int UserGuildSettingsVersion = -1;

    friend void to_json(nlohmann::json &j, const ClientStateProperties &m);
};

struct IdentifyMessage : GatewayMessage {
    std::string Token;
    IdentifyProperties Properties;
    PresenceData Presence;
    ClientStateProperties ClientState;
    bool DoesSupportCompression = false;
    int Capabilities;

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

struct GuildMemberUpdateMessage {
    Snowflake GuildID;            //
    std::vector<Snowflake> Roles; //
    UserData User;                //
    std::string Nick;             // opt, null
    std::string JoinedAt;
    std::string PremiumSince; // opt, null

    friend void from_json(const nlohmann::json &j, GuildMemberUpdateMessage &m);
};

struct ClientStatusData {
    std::optional<std::string> Desktop;
    std::optional<std::string> Mobile;
    std::optional<std::string> Web;

    friend void from_json(const nlohmann::json &j, ClientStatusData &m);
};

struct PresenceUpdateMessage {
    nlohmann::json User; // the client updates an existing object from this data
    std::optional<Snowflake> GuildID;
    std::string StatusMessage;
    std::vector<ActivityData> Activities;
    ClientStatusData ClientStatus;

    friend void from_json(const nlohmann::json &j, PresenceUpdateMessage &m);
};

struct CreateDMObject {
    std::vector<Snowflake> Recipients;

    friend void to_json(nlohmann::json &j, const CreateDMObject &m);
};

struct ResumeMessage : GatewayMessage {
    std::string Token;
    std::string SessionID;
    int Sequence;

    friend void to_json(nlohmann::json &j, const ResumeMessage &m);
};

struct GuildRoleUpdateObject {
    Snowflake GuildID;
    RoleData Role;

    friend void from_json(const nlohmann::json &j, GuildRoleUpdateObject &m);
};

struct GuildRoleCreateObject {
    Snowflake GuildID;
    RoleData Role;

    friend void from_json(const nlohmann::json &j, GuildRoleCreateObject &m);
};

struct GuildRoleDeleteObject {
    Snowflake GuildID;
    Snowflake RoleID;

    friend void from_json(const nlohmann::json &j, GuildRoleDeleteObject &m);
};

struct MessageReactionAddObject {
    Snowflake UserID;
    Snowflake ChannelID;
    Snowflake MessageID;
    std::optional<Snowflake> GuildID;
    std::optional<GuildMember> Member;
    EmojiData Emoji;

    friend void from_json(const nlohmann::json &j, MessageReactionAddObject &m);
};

struct MessageReactionRemoveObject {
    Snowflake UserID;
    Snowflake ChannelID;
    Snowflake MessageID;
    std::optional<Snowflake> GuildID;
    EmojiData Emoji;

    friend void from_json(const nlohmann::json &j, MessageReactionRemoveObject &m);
};

struct ChannelRecipientAdd {
    UserData User;
    Snowflake ChannelID;

    friend void from_json(const nlohmann::json &j, ChannelRecipientAdd &m);
};

struct ChannelRecipientRemove {
    UserData User;
    Snowflake ChannelID;

    friend void from_json(const nlohmann::json &j, ChannelRecipientRemove &m);
};

struct TypingStartObject {
    Snowflake ChannelID;
    std::optional<Snowflake> GuildID;
    Snowflake UserID;
    uint64_t Timestamp;
    std::optional<GuildMember> Member;

    friend void from_json(const nlohmann::json &j, TypingStartObject &m);
};

// implement rest as needed
struct ModifyGuildObject {
    std::optional<std::string> Name;
    std::optional<std::string> IconData;

    friend void to_json(nlohmann::json &j, const ModifyGuildObject &m);
};

struct GuildBanRemoveObject {
    Snowflake GuildID;
    UserData User;

    friend void from_json(const nlohmann::json &j, GuildBanRemoveObject &m);
};

struct GuildBanAddObject {
    Snowflake GuildID;
    UserData User;

    friend void from_json(const nlohmann::json &j, GuildBanAddObject &m);
};

struct InviteCreateObject {
    Snowflake ChannelID;
    std::string Code;
    std::string CreatedAt;
    std::optional<Snowflake> GuildID;
    std::optional<UserData> Inviter;
    int MaxAge;
    int MaxUses;
    UserData TargetUser;
    std::optional<ETargetUserType> TargetUserType;
    bool IsTemporary;
    int Uses;

    friend void from_json(const nlohmann::json &j, InviteCreateObject &m);
};

struct InviteDeleteObject {
    Snowflake ChannelID;
    std::optional<Snowflake> GuildID;
    std::string Code;

    friend void from_json(const nlohmann::json &j, InviteDeleteObject &m);
};
