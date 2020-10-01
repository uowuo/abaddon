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

// most stuff below should just be objects that get processed and thrown away immediately

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
    GUILD_CREATE,
    GUILD_DELETE,
    MESSAGE_DELETE_BULK,
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

struct ReadyEventData {
    int GatewayVersion;                   //
    User User;                            //
    std::vector<Guild> Guilds;            //
    std::string SessionID;                //
    std::vector<Channel> PrivateChannels; //

    // undocumented
    std::string AnalyticsToken; // opt
    int FriendSuggestionCount;  // opt
    UserSettings UserSettings;  // opt
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
