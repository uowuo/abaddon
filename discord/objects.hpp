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

// most stuff below should just be objects that get processed and thrown away immediately

enum class GatewayOp : int {
    Event = 0,
    Heartbeat = 1,
    Identify = 2,
    UpdateStatus = 3,
    Resume = 6,
    Reconnect = 7,
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
        User User;                    //
        std::vector<Snowflake> Roles; //
        // PresenceData Presence; //
        std::string PremiumSince; // opt
        std::string Nickname;     // opt
        bool IsMuted;             //
        std::string JoinedAt;     //
        std::string HoistedRole;  // null
        bool IsDefeaned;          //

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
    std::vector<Activity> Activities; // null (but never sent as such)
    std::string Status;
    bool IsAFK;
    int Since = 0;

    friend void to_json(nlohmann::json &j, const UpdateStatusMessage &m);
};

struct ReadyEventData {
    int GatewayVersion;
    User SelfUser;
    std::vector<Guild> Guilds;
    std::string SessionID;
    std::vector<Channel> PrivateChannels;

    // undocumented
    std::optional<std::vector<User>> Users;
    std::optional<std::string> AnalyticsToken;
    std::optional<int> FriendSuggestionCount;
    UserSettings UserSettings;
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
    UpdateStatusMessage Presence;
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
    User User;                    //
    std::string Nick;             // opt, null
    std::string JoinedAt;
    std::string PremiumSince; // opt, null

    friend void from_json(const nlohmann::json &j, GuildMemberUpdateMessage &m);
};

struct ClientStatus {
    std::string Desktop; // opt
    std::string Mobile;  // opt
    std::string Web;     // opt

    friend void from_json(const nlohmann::json &j, ClientStatus &m);
};

struct PresenceUpdateMessage {
    nlohmann::json User; // the client updates an existing object from this data
    Snowflake GuildID;   // opt
    std::string Status;
    // std::vector<Activity> Activities;
    ClientStatus ClientStatus;

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
    Role Role;

    friend void from_json(const nlohmann::json &j, GuildRoleUpdateObject &m);
};

struct GuildRoleCreateObject {
    Snowflake GuildID;
    Role Role;

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
    Emoji Emoji;

    friend void from_json(const nlohmann::json &j, MessageReactionAddObject &m);
};

struct MessageReactionRemoveObject {
    Snowflake UserID;
    Snowflake ChannelID;
    Snowflake MessageID;
    std::optional<Snowflake> GuildID;
    Emoji Emoji;

    friend void from_json(const nlohmann::json &j, MessageReactionRemoveObject &m);
};
