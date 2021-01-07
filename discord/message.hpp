#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include "user.hpp"
#include "sticker.hpp"
#include "emoji.hpp"
#include <string>
#include <vector>

enum class MessageType {
    DEFAULT = 0,                                 // yep
    RECIPIENT_ADD = 1,                           // nope
    RECIPIENT_REMOVE = 2,                        // nope
    CALL = 3,                                    // nope
    CHANNEL_NAME_CHANGE = 4,                     // nope
    CHANNEL_ICON_CHANGE = 5,                     // nope
    CHANNEL_PINNED_MESSAGE = 6,                  // yep
    GUILD_MEMBER_JOIN = 7,                       // yep
    USER_PREMIUM_GUILD_SUBSCRIPTION = 8,         // nope
    USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_1 = 9,  // nope
    USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_2 = 10, // nope
    USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_3 = 11, // nope
    CHANNEL_FOLLOW_ADD = 12,                     // nope
    GUILD_DISCOVERY_DISQUALIFIED = 14,           // nope
    GUILD_DISCOVERY_REQUALIFIED = 15,            // nope
    INLINE_REPLY = 19,                           // kinda
    APPLICATION_COMMAND = 20,                    // yep
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
    std::string Text;
    std::optional<std::string> IconURL;
    std::optional<std::string> ProxyIconURL;

    friend void to_json(nlohmann::json &j, const EmbedFooterData &m);
    friend void from_json(const nlohmann::json &j, EmbedFooterData &m);
};

struct EmbedImageData {
    std::optional<std::string> URL;
    std::optional<std::string> ProxyURL;
    std::optional<int> Height;
    std::optional<int> Width;

    friend void to_json(nlohmann::json &j, const EmbedImageData &m);
    friend void from_json(const nlohmann::json &j, EmbedImageData &m);
};

struct EmbedThumbnailData {
    std::optional<std::string> URL;
    std::optional<std::string> ProxyURL;
    std::optional<int> Height;
    std::optional<int> Width;

    friend void to_json(nlohmann::json &j, const EmbedThumbnailData &m);
    friend void from_json(const nlohmann::json &j, EmbedThumbnailData &m);
};

struct EmbedVideoData {
    std::optional<std::string> URL;
    std::optional<int> Height;
    std::optional<int> Width;

    friend void to_json(nlohmann::json &j, const EmbedVideoData &m);
    friend void from_json(const nlohmann::json &j, EmbedVideoData &m);
};

struct EmbedProviderData {
    std::optional<std::string> Name;
    std::optional<std::string> URL; // null

    friend void to_json(nlohmann::json &j, const EmbedProviderData &m);
    friend void from_json(const nlohmann::json &j, EmbedProviderData &m);
};

struct EmbedAuthorData {
    std::optional<std::string> Name;
    std::optional<std::string> URL;
    std::optional<std::string> IconURL;
    std::optional<std::string> ProxyIconURL;

    friend void to_json(nlohmann::json &j, const EmbedAuthorData &m);
    friend void from_json(const nlohmann::json &j, EmbedAuthorData &m);
};

struct EmbedFieldData {
    std::string Name;
    std::string Value;
    std::optional<bool> Inline;

    friend void to_json(nlohmann::json &j, const EmbedFieldData &m);
    friend void from_json(const nlohmann::json &j, EmbedFieldData &m);
};

struct EmbedData {
    std::optional<std::string> Title;
    std::optional<std::string> Type;
    std::optional<std::string> Description;
    std::optional<std::string> URL;
    std::optional<std::string> Timestamp;
    std::optional<int> Color;
    std::optional<EmbedFooterData> Footer;
    std::optional<EmbedImageData> Image;
    std::optional<EmbedThumbnailData> Thumbnail;
    std::optional<EmbedVideoData> Video;
    std::optional<EmbedProviderData> Provider;
    std::optional<EmbedAuthorData> Author;
    std::optional<std::vector<EmbedFieldData>> Fields;

    friend void to_json(nlohmann::json &j, const EmbedData &m);
    friend void from_json(const nlohmann::json &j, EmbedData &m);
};

struct AttachmentData {
    Snowflake ID;
    std::string Filename;
    int Bytes;
    std::string URL;
    std::string ProxyURL;
    std::optional<int> Height; // null
    std::optional<int> Width;  // null

    friend void to_json(nlohmann::json &j, const AttachmentData &m);
    friend void from_json(const nlohmann::json &j, AttachmentData &m);
};

struct MessageReferenceData {
    std::optional<Snowflake> MessageID;
    std::optional<Snowflake> ChannelID;
    std::optional<Snowflake> GuildID;

    friend void from_json(const nlohmann::json &j, MessageReferenceData &m);
    friend void to_json(nlohmann::json &j, const MessageReferenceData &m);
};

struct ReactionData {
    int Count;
    bool HasReactedWith;
    EmojiData Emoji;

    friend void from_json(const nlohmann::json &j, ReactionData &m);
    friend void to_json(nlohmann::json &j, const ReactionData &m);
};

struct MessageApplicationData {
    Snowflake ID;
    std::optional<std::string> CoverImage;
    std::string Description;
    std::string Icon; // null
    std::string Name;

    friend void from_json(const nlohmann::json &j, MessageApplicationData &m);
    friend void to_json(nlohmann::json &j, const MessageApplicationData &m);
};

struct Message {
    Snowflake ID;
    Snowflake ChannelID;
    std::optional<Snowflake> GuildID;
    UserData Author;
    // std::optional<GuildMember> Member;
    std::string Content;
    std::string Timestamp;
    std::string EditedTimestamp; // null
    bool IsTTS;
    bool DoesMentionEveryone;
    std::vector<UserData> Mentions; // full user accessible
    // std::vector<RoleData> MentionRoles;
    // std::optional<std::vector<ChannelMentionData>> MentionChannels;
    std::vector<AttachmentData> Attachments;
    std::vector<EmbedData> Embeds;
    std::optional<std::vector<ReactionData>> Reactions;
    std::optional<std::string> Nonce;
    bool IsPinned;
    std::optional<Snowflake> WebhookID;
    MessageType Type;
    // std::optional<MessageActivityData> ActivityData;
    std::optional<MessageApplicationData> Application;
    std::optional<MessageReferenceData> MessageReference;
    std::optional<MessageFlags> Flags = MessageFlags::NONE;
    std::optional<std::vector<StickerData>> Stickers;
    std::optional<std::shared_ptr<Message>> ReferencedMessage; // has_value && null means deleted

    friend void from_json(const nlohmann::json &j, Message &m);
    void from_json_edited(const nlohmann::json &j); // for MESSAGE_UPDATE

    // custom fields to track changes
    void SetDeleted();
    void SetEdited();
    bool IsDeleted() const;
    bool IsEdited() const;

private:
    bool m_deleted = false;
    bool m_edited = false;
};
