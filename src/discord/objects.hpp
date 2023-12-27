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
#include "relationship.hpp"
#include "errors.hpp"

// most stuff below should just be objects that get processed and thrown away immediately

enum class GatewayOp : int {
    Dispatch = 0,
    Heartbeat = 1,
    Identify = 2,
    PresenceUpdate = 3,
    VoiceStateUpdate = 4,
    VoiceServerPing = 5,
    Resume = 6,
    Reconnect = 7,
    RequestGuildMembers = 8,
    InvalidSession = 9,
    Hello = 10,
    HeartbeatAck = 11,
    // 12 unused
    CallConnect = 13,
    GuildSubscriptions = 14,
    LobbyConnect = 15,
    LobbyDisconnect = 16,
    LobbyVoiceStatesUpdate = 17,
    StreamCreate = 18,
    StreamDelete = 19,
    StreamWatch = 20,
    StreamPing = 21,
    StreamSetPaused = 22,
    // 23 unused
    RequestGuildApplicationCommands = 24,
    EmbeddedActivityLaunch = 25,
    EmbeddedActivityClose = 26,
    EmbeddedActivityUpdate = 27,
    RequestForumUnreads = 28,
    RemoteCommand = 29,
    GetDeletedEntityIDsNotMatchingHash = 30,
    RequestSoundboardSounds = 31,
    SpeedTestCreate = 32,
    SpeedTestDelete = 33,
    RequestLastMessages = 34,
    SearchRecentMembers = 35,
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
    USER_NOTE_UPDATE,
    READY_SUPPLEMENTAL,
    GUILD_EMOJIS_UPDATE,
    GUILD_JOIN_REQUEST_CREATE,
    GUILD_JOIN_REQUEST_UPDATE,
    GUILD_JOIN_REQUEST_DELETE,
    RELATIONSHIP_REMOVE,
    RELATIONSHIP_ADD,
    THREAD_CREATE,
    THREAD_UPDATE,
    THREAD_DELETE,
    THREAD_LIST_SYNC,
    THREAD_MEMBER_UPDATE,
    THREAD_MEMBERS_UPDATE,
    THREAD_MEMBER_LIST_UPDATE,
    MESSAGE_ACK,
    USER_GUILD_SETTINGS_UPDATE,
    GUILD_MEMBERS_CHUNK,
    VOICE_STATE_UPDATE,
    VOICE_SERVER_UPDATE,
    CALL_CREATE,
};

enum class GatewayCloseCode : uint16_t {
    // standard
    Normal = 1000,
    GoingAway = 1001,
    ProtocolError = 1002,
    Unsupported = 1003,
    NoStatus = 1005,
    Abnormal = 1006,
    UnsupportedPayload = 1007,
    PolicyViolation = 1008,
    TooLarge = 1009,
    MandatoryExtension = 1010,
    ServerError = 1011,
    ServiceRestart = 1012,
    TryAgainLater = 1013,
    BadGateway = 1014,
    TLSHandshakeFailed = 1015,

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
        virtual ~Item() = default;

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

        [[nodiscard]] GuildMember GetAsMemberData() const;

        friend void from_json(const nlohmann::json &j, MemberItem &m);

    private:
        GuildMember m_member_data;
    };

    struct OpObject {
        std::string Op;
        std::optional<int> Index;
        std::optional<std::vector<std::unique_ptr<Item>>> Items; // SYNC
        std::optional<std::pair<int, int>> Range;                // SYNC
        std::optional<std::unique_ptr<Item>> OpItem;             // UPDATE

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
    std::optional<bool> ShouldGetTyping;
    std::optional<bool> ShouldGetActivities;
    std::optional<bool> ShouldGetThreads;
    std::optional<std::vector<std::string>> Members;                               // snowflake?
    std::optional<std::map<Snowflake, std::vector<std::pair<int, int>>>> Channels; // channel ID -> range of sidebar
    std::optional<std::vector<Snowflake>> ThreadIDs;

    friend void to_json(nlohmann::json &j, const LazyLoadRequestMessage &m);
};

struct UpdateStatusMessage {
    int Since = 0;
    std::vector<ActivityData> Activities;
    PresenceStatus Status;
    bool IsAFK = false;

    friend void to_json(nlohmann::json &j, const UpdateStatusMessage &m);
};

struct RequestGuildMembersMessage {
    Snowflake GuildID;
    bool Presences;
    std::vector<Snowflake> UserIDs;

    friend void to_json(nlohmann::json &j, const RequestGuildMembersMessage &m);
};

struct ReadStateEntry {
    int MentionCount;
    Snowflake LastMessageID;
    Snowflake ID;
    // std::string LastPinTimestamp; iso

    friend void from_json(const nlohmann::json &j, ReadStateEntry &m);
    friend void to_json(nlohmann::json &j, const ReadStateEntry &m);
};

struct ReadStateData {
    int Version;
    bool IsPartial;
    std::vector<ReadStateEntry> Entries;

    friend void from_json(const nlohmann::json &j, ReadStateData &m);
};

enum class NotificationLevel {
    ALL_MESSAGES = 0,
    ONLY_MENTIONS = 1,
    NO_MESSAGES = 2,
    USE_UPPER = 3, // actually called "NULL"
};

struct UserGuildSettingsChannelOverride {
    bool Muted;
    MuteConfigData MuteConfig;
    NotificationLevel MessageNotifications;
    bool Collapsed;
    Snowflake ChannelID;

    friend void from_json(const nlohmann::json &j, UserGuildSettingsChannelOverride &m);
    friend void to_json(nlohmann::json &j, const UserGuildSettingsChannelOverride &m);
};

struct UserGuildSettingsEntry {
    int Version;
    bool SuppressRoles;
    bool SuppressEveryone;
    bool Muted;
    MuteConfigData MuteConfig;
    bool MobilePush;
    NotificationLevel MessageNotifications;
    bool HideMutedChannels;
    Snowflake GuildID;
    std::vector<UserGuildSettingsChannelOverride> ChannelOverrides;

    friend void from_json(const nlohmann::json &j, UserGuildSettingsEntry &m);
    friend void to_json(nlohmann::json &j, const UserGuildSettingsEntry &m);

    std::optional<UserGuildSettingsChannelOverride> GetOverride(Snowflake channel_id) const;
};

struct UserGuildSettingsData {
    int Version;
    bool IsPartial;
    std::map<Snowflake, UserGuildSettingsEntry> Entries;

    friend void from_json(const nlohmann::json &j, UserGuildSettingsData &m);
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
    std::optional<std::vector<RelationshipData>> Relationships;
    std::optional<std::vector<GuildApplicationData>> GuildJoinRequests;
    ReadStateData ReadState;
    UserGuildSettingsData GuildSettings;
    // std::vector<Unknown> ConnectedAccounts; // opt
    // std::map<std::string, Unknown> Consents; // opt
    // std::vector<Unknown> Experiments; // opt
    // std::vector<Unknown> GuildExperiments; // opt
    // std::map<Unknown, Unknown> Notes; // opt
    // std::vector<PresenceData> Presences; // opt
    // std::vector<ReadStateData> ReadStates; // opt
    // Unknown Tutorial; // opt, null
    // std::vector<GuildSettingData> UserGuildSettings; // opt

    friend void from_json(const nlohmann::json &j, ReadyEventData &m);
};

struct MergedPresence {
    Snowflake UserID;
    std::optional<uint64_t> LastModified;
    PresenceData Presence;

    friend void from_json(const nlohmann::json &j, MergedPresence &m);
};

struct SupplementalMergedPresencesData {
    std::vector<std::vector<MergedPresence>> Guilds;
    std::vector<MergedPresence> Friends;

    friend void from_json(const nlohmann::json &j, SupplementalMergedPresencesData &m);
};

struct VoiceState;
struct SupplementalGuildEntry {
    // std::vector<?> EmbeddedActivities;
    Snowflake ID;
    std::vector<VoiceState> VoiceStates;

    friend void from_json(const nlohmann::json &j, SupplementalGuildEntry &m);
};

struct ReadySupplementalData {
    SupplementalMergedPresencesData MergedPresences;
    std::vector<SupplementalGuildEntry> Guilds;

    friend void from_json(const nlohmann::json &j, ReadySupplementalData &m);
};

struct IdentifyProperties {
    std::string OS;
    std::string Browser;
    std::string Device;
    std::string SystemLocale;
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
    int UserSettingsVersion = -1;

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

struct CreateMessageAttachmentObject {
    int ID;
    std::optional<std::string> Description;

    friend void to_json(nlohmann::json &j, const CreateMessageAttachmentObject &m);
};

struct CreateMessageObject {
    std::string Content;
    MessageFlags Flags = MessageFlags::NONE;
    std::optional<MessageReferenceData> MessageReference;
    std::optional<std::string> Nonce;
    std::optional<std::vector<CreateMessageAttachmentObject>> Attachments;

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

struct ConnectionData {
    std::string ID;
    std::string Type;
    std::string Name;
    bool IsVerified;

    friend void from_json(const nlohmann::json &j, ConnectionData &m);
};

struct MutualGuildData {
    Snowflake ID;
    std::optional<std::string> Nick; // null

    friend void from_json(const nlohmann::json &j, MutualGuildData &m);
};

struct UserProfileData {
    std::vector<ConnectionData> ConnectedAccounts;
    std::vector<MutualGuildData> MutualGuilds;
    std::optional<std::string> PremiumGuildSince; // null
    std::optional<std::string> PremiumSince;      // null
    std::optional<std::string> LegacyUsername;    // null
    UserData User;

    friend void from_json(const nlohmann::json &j, UserProfileData &m);
};

struct UserNoteObject {
    // idk if these can be null or missing but i play it safe
    std::optional<std::string> Note;
    std::optional<Snowflake> NoteUserID;
    std::optional<Snowflake> UserID;

    friend void from_json(const nlohmann::json &j, UserNoteObject &m);
};

struct UserSetNoteObject {
    std::string Note;

    friend void to_json(nlohmann::json &j, const UserSetNoteObject &m);
};

struct UserNoteUpdateMessage {
    std::string Note;
    Snowflake ID;

    friend void from_json(const nlohmann::json &j, UserNoteUpdateMessage &m);
};

struct RelationshipsData {
    std::vector<UserData> Users;

    friend void from_json(const nlohmann::json &j, RelationshipsData &m);
};

struct ModifyGuildMemberObject {
    // std::optional<std::string> Nick;
    // std::optional<bool> IsMuted;
    // std::optional<bool> IsDeaf;
    // std::optional<Snowflake> ChannelID;

    std::optional<std::vector<Snowflake>> Roles;

    friend void to_json(nlohmann::json &j, const ModifyGuildMemberObject &m);
};

struct ModifyGuildRoleObject {
    std::optional<std::string> Name;
    std::optional<Permission> Permissions;
    std::optional<uint32_t> Color;
    std::optional<bool> IsHoisted;
    std::optional<bool> Mentionable;

    friend void to_json(nlohmann::json &j, const ModifyGuildRoleObject &m);
};

struct ModifyGuildRolePositionsObject {
    struct PositionParam {
        Snowflake ID;
        std::optional<int> Position; // no idea why this can be optional

        friend void to_json(nlohmann::json &j, const PositionParam &m);
    };
    std::vector<PositionParam> Positions;

    friend void to_json(nlohmann::json &j, const ModifyGuildRolePositionsObject &m);
};

struct GuildEmojisUpdateObject {
    Snowflake GuildID;
    // std::vector<EmojiData> Emojis;
    // GuildHashes, undocumented

    friend void from_json(const nlohmann::json &j, GuildEmojisUpdateObject &m);
};

struct ModifyGuildEmojiObject {
    std::optional<std::string> Name;
    // std::optional<std::vector<Snowflake>> Roles;

    friend void to_json(nlohmann::json &j, const ModifyGuildEmojiObject &m);
};

struct GuildJoinRequestCreateData {
    GuildApplicationStatus Status;
    GuildApplicationData Request;
    Snowflake GuildID;

    friend void from_json(const nlohmann::json &j, GuildJoinRequestCreateData &m);
};

using GuildJoinRequestUpdateData = GuildJoinRequestCreateData;

struct GuildJoinRequestDeleteData {
    Snowflake UserID;
    Snowflake GuildID;

    friend void from_json(const nlohmann::json &j, GuildJoinRequestDeleteData &m);
};

struct VerificationFieldObject {
    std::string Type;
    std::string Label;
    bool Required;
    std::vector<std::string> Values;
    std::optional<bool> Response; // present in client to server

    friend void from_json(const nlohmann::json &j, VerificationFieldObject &m);
    friend void to_json(nlohmann::json &j, const VerificationFieldObject &m);
};

struct VerificationGateInfoObject {
    std::optional<std::string> Description;
    std::optional<std::vector<VerificationFieldObject>> VerificationFields;
    std::optional<std::string> Version;
    std::optional<bool> Enabled; // present only in client to server in modify gate

    friend void from_json(const nlohmann::json &j, VerificationGateInfoObject &m);
    friend void to_json(nlohmann::json &j, const VerificationGateInfoObject &m);
};

// not sure what the structure for this really is
struct RateLimitedResponse {
    int Code;
    bool Global;
    std::optional<std::string> Message;
    float RetryAfter;

    friend void from_json(const nlohmann::json &j, RateLimitedResponse &m);
};

struct RelationshipRemoveData {
    Snowflake ID;
    RelationshipType Type;

    friend void from_json(const nlohmann::json &j, RelationshipRemoveData &m);
};

struct RelationshipAddData {
    Snowflake ID;
    // Nickname; same deal as the other comment somewhere else
    RelationshipType Type;
    UserData User;
    // std::optional<bool> ShouldNotify; // i guess if the client should send a notification. not worth caring about

    friend void from_json(const nlohmann::json &j, RelationshipAddData &m);
};

struct FriendRequestObject {
    std::string Username;
    int Discriminator;

    friend void to_json(nlohmann::json &j, const FriendRequestObject &m);
};

struct PutRelationshipObject {
    std::optional<RelationshipType> Type;

    friend void to_json(nlohmann::json &j, const PutRelationshipObject &m);
};

struct ThreadCreateData {
    ChannelData Channel;

    friend void from_json(const nlohmann::json &j, ThreadCreateData &m);
};

struct ThreadDeleteData {
    Snowflake ID;
    Snowflake GuildID;
    Snowflake ParentID;
    ChannelType Type;

    friend void from_json(const nlohmann::json &j, ThreadDeleteData &m);
};

// pretty different from docs
struct ThreadListSyncData {
    std::vector<ChannelData> Threads;
    Snowflake GuildID;
    // std::optional<std::vector<???>> MostRecentMessages;

    friend void from_json(const nlohmann::json &j, ThreadListSyncData &m);
};

struct ThreadMembersUpdateData {
    Snowflake ID;
    Snowflake GuildID;
    int MemberCount;
    std::optional<std::vector<ThreadMemberObject>> AddedMembers;
    std::optional<std::vector<Snowflake>> RemovedMemberIDs;

    friend void from_json(const nlohmann::json &j, ThreadMembersUpdateData &m);
};

struct ArchivedThreadsResponseData {
    std::vector<ChannelData> Threads;
    std::vector<ThreadMemberObject> Members;
    bool HasMore;

    friend void from_json(const nlohmann::json &j, ArchivedThreadsResponseData &m);
};

struct ThreadMemberUpdateData {
    ThreadMemberObject Member;

    friend void from_json(const nlohmann::json &j, ThreadMemberUpdateData &m);
};

struct ThreadUpdateData {
    ChannelData Thread;

    friend void from_json(const nlohmann::json &j, ThreadUpdateData &m);
};

struct ThreadMemberListUpdateData {
    struct UserEntry {
        Snowflake UserID;
        // PresenceData Presence;
        GuildMember Member;

        friend void from_json(const nlohmann::json &j, UserEntry &m);
    };

    Snowflake ThreadID;
    Snowflake GuildID;
    std::vector<UserEntry> Members;

    friend void from_json(const nlohmann::json &j, ThreadMemberListUpdateData &m);
};

struct ModifyChannelObject {
    std::optional<bool> Archived;
    std::optional<bool> Locked;

    friend void to_json(nlohmann::json &j, const ModifyChannelObject &m);
};

struct MessageAckData {
    // int Version; // what is this ?!?!?!!?
    Snowflake MessageID;
    Snowflake ChannelID;

    friend void from_json(const nlohmann::json &j, MessageAckData &m);
};

struct AckBulkData {
    std::vector<ReadStateEntry> ReadStates;

    friend void to_json(nlohmann::json &j, const AckBulkData &m);
};

struct UserGuildSettingsUpdateData {
    UserGuildSettingsEntry Settings;

    friend void from_json(const nlohmann::json &j, UserGuildSettingsUpdateData &m);
};

struct GuildMembersChunkData {
    /*
    not needed so not deserialized
    int ChunkCount;
    int ChunkIndex;
    std::vector<?> NotFound;
    */
    Snowflake GuildID;
    std::vector<GuildMember> Members;

    friend void from_json(const nlohmann::json &j, GuildMembersChunkData &m);
};

struct VoiceState {
    std::optional<Snowflake> ChannelID;
    bool IsDeafened;
    bool IsMuted;
    std::optional<Snowflake> GuildID;
    std::optional<GuildMember> Member;
    bool IsSelfDeafened;
    bool IsSelfMuted;
    bool IsSelfVideo;
    bool IsSelfStream = false;
    std::string SessionID;
    bool IsSuppressed;
    Snowflake UserID;

    friend void from_json(const nlohmann::json &j, VoiceState &m);
};

#ifdef WITH_VOICE
struct VoiceStateUpdateMessage {
    std::optional<Snowflake> GuildID;
    std::optional<Snowflake> ChannelID;
    bool SelfMute = false;
    bool SelfDeaf = false;
    bool SelfVideo = false;
    std::string PreferredRegion;

    friend void to_json(nlohmann::json &j, const VoiceStateUpdateMessage &m);
};

struct VoiceServerUpdateData {
    std::string Token;
    std::string Endpoint;
    std::optional<Snowflake> GuildID;
    std::optional<Snowflake> ChannelID;

    friend void from_json(const nlohmann::json &j, VoiceServerUpdateData &m);
};

struct CallCreateData {
    Snowflake ChannelID;
    std::vector<VoiceState> VoiceStates;
    // Snowflake MessageID;
    // std::string Region;
    // std::vector<?> Ringing;
    // std::vector<?> EmbeddedActivities;

    friend void from_json(const nlohmann::json &j, CallCreateData &m);
};
#endif
