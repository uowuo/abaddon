#pragma once
#include <string>
#include <vector>
#include "snowflake.hpp"
#include "json.hpp"
#include "user.hpp"
#include "sticker.hpp"
#include "emoji.hpp"
#include "member.hpp"
#include "interactions.hpp"
#include "webhook.hpp"
#include "misc/bitwise.hpp"

enum class MessageType {
    DEFAULT = 0,                                       // yep
    RECIPIENT_ADD = 1,                                 // yep
    RECIPIENT_REMOVE = 2,                              // yep
    CALL = 3,                                          // yep (sorta)
    CHANNEL_NAME_CHANGE = 4,                           // yep
    CHANNEL_ICON_CHANGE = 5,                           // yep
    CHANNEL_PINNED_MESSAGE = 6,                        // yep
    GUILD_MEMBER_JOIN = 7,                             // yep
    USER_PREMIUM_GUILD_SUBSCRIPTION = 8,               // yep
    USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_1 = 9,        // yep
    USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_2 = 10,       // yep
    USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_3 = 11,       // yep
    CHANNEL_FOLLOW_ADD = 12,                           // yep
    GUILD_DISCOVERY_DISQUALIFIED = 14,                 // yep
    GUILD_DISCOVERY_REQUALIFIED = 15,                  // yep
    GUILD_DISCOVERY_GRACE_PERIOD_INITIAL_WARNING = 16, // yep
    GUILD_DISCOVERY_GRACE_PERIOD_FINAL_WARNING = 17,   // yep
    THREAD_CREATED = 18,                               // yep
    INLINE_REPLY = 19,                                 // yep
    APPLICATION_COMMAND = 20,                          // yep
    THREAD_STARTER_MESSAGE = 21,                       // nope
    GUILD_INVITE_REMINDER = 22,                        // nope
    CONTEXT_MENU_COMMAND = 23,                         // nope
    AUTO_MODERATION_ACTION = 24,                       // nope
    ROLE_SUBSCRIPTION_PURCHASE = 25,                   // nope
    INTERACTION_PREMIUM_UPSELL = 26,                   // nope
    STAGE_START = 27,                                  // nope
    STAGE_END = 28,                                    // nope
    STAGE_SPEAKER = 29,                                // nope
    STAGE_TOPIC = 31,                                  // nope
    GUILD_APPLICATION_PREMIUM_SUBSCRIPTION = 32,       // nope
    PRIVATE_CHANNEL_INTEGRATION_ADDED = 33,            // nope
    PRIVATE_CHANNEL_INTEGRATION_REMOVED = 34,          // nope
    PREMIUM_REFERRAL = 35,                             // nope
    GUILD_INCIDENT_ALERT_MODE_ENABLED = 36,            // nope
    GUILD_INCIDENT_ALERT_MODE_DISABLED = 37,           // nope
    GUILD_INCIDENT_REPORT_RAID = 38,                   // nope
    GUILD_INCIDENT_REPORT_FALSE_ALARM = 39,            // nope
    GUILD_DEADCHAT_REVIVE_PROMPT = 40,                 // nope
    CUSTOM_GIFT = 41,                                  // nope
    GUILD_GAMING_STATS_PROMPT = 42,                    // nope
    POLL = 43,                                         // nope
    PURCHASE_NOTIFICATION = 44,                        // nope
};

enum class MessageFlags {
    NONE = 0,
    CROSSPOSTED = 1 << 0,                            // this message has been published to subscribed channels (via Channel Following)
    IS_CROSSPOST = 1 << 1,                           // this message originated from a message in another channel (via Channel Following)
    SUPPRESS_EMBEDS = 1 << 2,                        // do not include any embeds when serializing this message
    SOURCE_MESSAGE_DELETE = 1 << 3,                  // the source message for this crosspost has been deleted (via Channel Following)
    URGENT = 1 << 4,                                 // this message came from the urgent message system
    HAS_THREAD = 1 << 5,                             // this message has an associated thread, with the same id as the message
    EPHEMERAL = 1 << 6,                              // this message is only visible to the user who invoked the Interaction
    LOADING = 1 << 7,                                // this message is an Interaction Response and the bot is "thinking"
    FAILED_TO_MENTION_SOME_ROLES_IN_THREAD = 1 << 8, // this message failed to mention some roles and add their members to the thread
    SHOULD_SHOW_LINK_NOT_DISCORD_WARNING = 1 << 10,  //
    SUPPRESS_NOTIFICATIONS = 1 << 12,                // this message will not trigger push and desktop notifications
    IS_VOICE_MESSAGE = 1 << 13,                      // this message is a voice message
};

template<>
struct Bitwise<MessageFlags> {
    static const bool enable = true;
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
    std::optional<int> Height;              // null
    std::optional<int> Width;               // null
    std::optional<std::string> Description; // alt text

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
    std::optional<GuildMember> Member;
    std::string Content;
    std::string Timestamp;
    std::string EditedTimestamp; // null
    bool IsTTS;
    bool DoesMentionEveryone;
    std::vector<UserData> Mentions; // full user accessible
    std::vector<Snowflake> MentionRoles;
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
    std::optional<MessageInteractionData> Interaction;
    std::optional<std::vector<StickerItem>> StickerItems;

    friend void from_json(const nlohmann::json &j, Message &m);
    void from_json_edited(const nlohmann::json &j); // for MESSAGE_UPDATE

    // custom fields to track changes
    bool IsPending = false; // for user-sent messages yet to be received in a MESSAGE_CREATE

    void SetDeleted();
    void SetEdited();
    bool IsDeleted() const;
    bool IsEdited() const;

    bool IsEditable() const noexcept;

    bool DoesMentionEveryoneOrUser(Snowflake id) const noexcept;
    bool DoesMention(Snowflake id) const noexcept;
    bool IsWebhook() const noexcept;

    std::optional<WebhookMessageData> GetWebhookData() const;

private:
    bool m_deleted = false;
    bool m_edited = false;
};
