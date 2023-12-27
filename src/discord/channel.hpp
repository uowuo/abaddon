#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include "user.hpp"
#include "permissions.hpp"
#include <string>
#include <vector>

enum class ChannelType : int {
    GUILD_TEXT = 0,
    DM = 1,
    GUILD_VOICE = 2,
    GROUP_DM = 3,
    GUILD_CATEGORY = 4,
    GUILD_NEWS = 5,
    GUILD_STORE = 6,
    /* 7 and 8 were used for LFG */
    /* 9 was used for threads */
    GUILD_NEWS_THREAD = 10,
    GUILD_PUBLIC_THREAD = 11,
    GUILD_PRIVATE_THREAD = 12,
    GUILD_STAGE_VOICE = 13,

    // Unimplemented:
    GUILD_DIRECTORY = 14,
    GUILD_FORUM = 15,
    GUILD_MEDIA = 16,
};

enum class StagePrivacy {
    PUBLIC = 1,
    GUILD_ONLY = 2,
};

constexpr const char *GetStagePrivacyDisplayString(StagePrivacy e) {
    switch (e) {
        case StagePrivacy::PUBLIC:
            return "Public";
        case StagePrivacy::GUILD_ONLY:
            return "Guild Only";
        default:
            return "Unknown";
    }
}

// should be moved somewhere?

struct ThreadMetadataData {
    bool IsArchived;
    int AutoArchiveDuration;
    std::string ArchiveTimestamp;
    std::optional<bool> IsLocked;

    friend void from_json(const nlohmann::json &j, ThreadMetadataData &m);
};

struct MuteConfigData {
    std::optional<std::string> EndTime; // nullopt is encoded as null
    int SelectedTimeWindow;

    friend void from_json(const nlohmann::json &j, MuteConfigData &m);
    friend void to_json(nlohmann::json &j, const MuteConfigData &m);
};

struct ThreadMemberObject {
    std::optional<Snowflake> ThreadID;
    std::optional<Snowflake> UserID;
    std::optional<bool> IsMuted;
    std::optional<MuteConfigData> MuteConfig;
    std::string JoinTimestamp;
    int Flags;

    friend void from_json(const nlohmann::json &j, ThreadMemberObject &m);
};

struct ChannelData {
    Snowflake ID;
    ChannelType Type;
    std::optional<Snowflake> GuildID;
    std::optional<int> Position;
    std::optional<std::vector<PermissionOverwrite>> PermissionOverwrites; // shouldnt be accessed
    std::optional<std::string> Name;                                      // null for dm's
    std::optional<std::string> Topic;                                     // null
    std::optional<bool> IsNSFW;
    std::optional<Snowflake> LastMessageID; // null
    std::optional<int> Bitrate;
    std::optional<int> UserLimit;
    std::optional<int> RateLimitPerUser;
    std::optional<std::vector<UserData>> Recipients; // only access id
    std::optional<std::vector<Snowflake>> RecipientIDs;
    std::optional<std::string> Icon; // null
    std::optional<Snowflake> OwnerID;
    std::optional<Snowflake> ApplicationID;
    std::optional<Snowflake> ParentID;           // null
    std::optional<std::string> LastPinTimestamp; // null
    std::optional<ThreadMetadataData> ThreadMetadata;
    std::optional<ThreadMemberObject> ThreadMember;

    friend void from_json(const nlohmann::json &j, ChannelData &m);
    void update_from_json(const nlohmann::json &j);

    [[nodiscard]] bool NSFW() const;
    [[nodiscard]] bool IsDM() const noexcept;
    [[nodiscard]] bool IsThread() const noexcept;
    [[nodiscard]] bool IsJoinedThread() const;
    [[nodiscard]] bool IsCategory() const noexcept;
    [[nodiscard]] bool IsText() const noexcept;
    [[nodiscard]] bool HasIcon() const noexcept;
    [[nodiscard]] std::string GetIconURL() const;
    [[nodiscard]] std::string GetDisplayName() const;
    [[nodiscard]] std::vector<Snowflake> GetChildIDs() const;
    [[nodiscard]] std::optional<PermissionOverwrite> GetOverwrite(Snowflake id) const;
    [[nodiscard]] std::vector<UserData> GetDMRecipients() const;
    [[nodiscard]] std::string GetRecipientsDisplay() const;
};
