#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include "user.hpp"
#include <string>
#include <vector>

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
    User Author;         //
    // GuildMember Member; // opt
    std::string Content;         //
    std::string Timestamp;       //
    std::string EditedTimestamp; // null
    bool IsTTS;                  //
    bool DoesMentionEveryone;    //
    std::vector<User> Mentions;  //
    // std::vector<Role> MentionRoles; //
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
