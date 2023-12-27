#pragma once
#include "snowflake.hpp"
#include "user.hpp"
#include "json.hpp"
#include "webhook.hpp"

enum class AuditLogActionType {
    GUILD_UPDATE = 1,
    CHANNEL_CREATE = 10,
    CHANNEL_UPDATE = 11,
    CHANNEL_DELETE = 12,
    CHANNEL_OVERWRITE_CREATE = 13,
    CHANNEL_OVERWRITE_UPDATE = 14,
    CHANNEL_OVERWRITE_DELETE = 15,
    MEMBER_KICK = 20,
    MEMBER_PRUNE = 21,
    MEMBER_BAN_ADD = 22,
    MEMBER_BAN_REMOVE = 23,
    MEMBER_UPDATE = 24,
    MEMBER_ROLE_UPDATE = 25,
    MEMBER_MOVE = 26,
    MEMBER_DISCONNECT = 27,
    BOT_ADD = 28,
    ROLE_CREATE = 30,
    ROLE_UPDATE = 31,
    ROLE_DELETE = 32,
    INVITE_CREATE = 40,
    INVITE_UPDATE = 41,
    INVITE_DELETE = 42,
    WEBHOOK_CREATE = 50,
    WEBHOOK_UPDATE = 51,
    WEBHOOK_DELETE = 52,
    EMOJI_CREATE = 60,
    EMOJI_UPDATE = 61,
    EMOJI_DELETE = 62,
    MESSAGE_DELETE = 72,
    MESSAGE_BULK_DELETE = 73,
    MESSAGE_PIN = 74,
    MESSAGE_UNPIN = 75,
    INTEGRATION_CREATE = 80,
    INTEGRATION_UPDATE = 81,
    INTEGRATION_DELETE = 82,
    STAGE_INSTANCE_CREATE = 83,
    STAGE_INSTANCE_UPDATE = 84,
    STAGE_INSTANCE_DELETE = 85,
    STICKER_CREATE = 90,
    STICKER_UPDATE = 91,
    STICKER_DELETE = 92,
    THREAD_CREATE = 110,
    THREAD_UPDATE = 111,
    THREAD_DELETE = 112,

    // Unhandled:
    APPLICATION_COMMAND_PERMISSION_UPDATE = 121,
    SOUNDBOARD_SOUND_CREATE = 130,
    SOUNDBOARD_SOUND_UPDATE = 131,
    SOUNDBOARD_SOUND_DELETE = 132,
    AUTO_MODERATION_RULE_CREATE = 140,
    AUTO_MODERATION_RULE_UPDATE = 141,
    AUTO_MODERATION_RULE_DELETE = 142,
    AUTO_MODERATION_BLOCK_MESSAGE = 143,
    AUTO_MODERATION_FLAG_TO_CHANNEL = 144,
    AUTO_MODERATION_USER_COMMUNICATION_DISABLED = 145,
    AUTO_MODERATION_QUARANTINE_USER = 146,
    CREATOR_MONETIZATION_REQUEST_CREATED = 150,
    CREATOR_MONETIZATION_TERMS_ACCEPTED = 151,
    ONBOARDING_PROMPT_CREATE = 163,
    ONBOARDING_PROMPT_UPDATE = 164,
    ONBOARDING_PROMPT_DELETE = 165,
    ONBOARDING_CREATE = 166,
    ONBOARDING_UPDATE = 167,
    GUILD_HOME_FEATURE_ITEM = 171,
    GUILD_HOME_REMOVE_ITEM = 172,
    HARMFUL_LINKS_BLOCKED_MESSAGE = 180,
    HOME_SETTINGS_CREATE = 190,
    HOME_SETTINGS_UPDATE = 191,
    VOICE_CHANNEL_STATUS_CREATE = 192,
    VOICE_CHANNEL_STATUS_DELETE = 193,
    CLYDE_AI_PROFILE_UPDATE = 194
};

struct AuditLogChange {
    std::string Key;
    std::optional<nlohmann::json> OldValue;
    std::optional<nlohmann::json> NewValue;

    friend void from_json(const nlohmann::json &j, AuditLogChange &m);
};

struct AuditLogOptions {
    std::optional<std::string> DeleteMemberDays; // MEMBER_PRUNE
    std::optional<std::string> MembersRemoved;   // MEMBER_PRUNE
    std::optional<Snowflake> ChannelID;          // MEMBER_MOVE, MESSAGE_PIN, MESSAGE_UNPIN, MESSAGE_DELETE
    std::optional<Snowflake> MessageID;          // MESSAGE_PIN, MESSAGE_UNPIN,
    std::optional<std::string> Count;            // MESSAGE_DELETE, MESSAGE_BULK_DELETE, MEMBER_DISCONNECT, MEMBER_MOVE
    std::optional<Snowflake> ID;                 // CHANNEL_OVERWRITE_CREATE, CHANNEL_OVERWRITE_UPDATE, CHANNEL_OVERWRITE_DELETE
    std::optional<std::string> Type;             // CHANNEL_OVERWRITE_CREATE, CHANNEL_OVERWRITE_UPDATE, CHANNEL_OVERWRITE_DELETE
    std::optional<std::string> RoleName;         // CHANNEL_OVERWRITE_CREATE, CHANNEL_OVERWRITE_UPDATE, CHANNEL_OVERWRITE_DELETE

    friend void from_json(const nlohmann::json &j, AuditLogOptions &m);
};

struct AuditLogEntry {
    Snowflake ID;
    std::string TargetID; // null
    std::optional<Snowflake> UserID;
    AuditLogActionType Type;
    std::optional<std::string> Reason;
    std::optional<std::vector<AuditLogChange>> Changes;
    std::optional<AuditLogOptions> Options;

    friend void from_json(const nlohmann::json &j, AuditLogEntry &m);

    template<typename T>
    std::optional<T> GetOldFromKey(const std::string &key) const;

    template<typename T>
    std::optional<T> GetNewFromKey(const std::string &key) const;
};

struct AuditLogData {
    std::vector<AuditLogEntry> Entries;
    std::vector<UserData> Users;
    std::vector<WebhookData> Webhooks;
    // std::vector<IntegrationData> Integrations;

    friend void from_json(const nlohmann::json &j, AuditLogData &m);
};

template<typename T>
inline std::optional<T> AuditLogEntry::GetOldFromKey(const std::string &key) const {
    if (!Changes.has_value()) return std::nullopt;
    for (const auto &change : *Changes)
        if (change.Key == key && change.OldValue.has_value())
            return change.OldValue->get<T>();

    return std::nullopt;
}

template<typename T>
inline std::optional<T> AuditLogEntry::GetNewFromKey(const std::string &key) const {
    if (!Changes.has_value()) return std::nullopt;
    for (const auto &change : *Changes)
        if (change.Key == key && change.NewValue.has_value())
            return change.NewValue->get<T>();

    return std::nullopt;
}
