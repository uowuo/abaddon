#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include <string>

enum class EPremiumType {
    None = 0,
    NitroClassic = 1,
    Nitro = 2,
    Basic = 3,
};

struct UserData {
    // todo: enum class? (for consistencys sake)
    enum {
        DiscordEmployee = 1 << 0,
        PartneredServerOwner = 1 << 1,
        HypeSquadEvents = 1 << 2,
        BugHunterLevel1 = 1 << 3,
        HouseBravery = 1 << 6,
        HouseBrilliance = 1 << 7,
        HouseBalance = 1 << 8,
        EarlySupporter = 1 << 9,
        TeamUser = 1 << 10, // no idea what this is
        System = 1 << 12,
        BugHunterLevel2 = 1 << 14,
        VerifiedBot = 1 << 16,
        EarlyVerifiedBotDeveloper = 1 << 17,
        CertifiedModerator = 1 << 18,
        BotHTTPInteractions = 1 << 19,
        Spammer = 1 << 20,
        DisablePremium = 1 << 21,
        ActiveDeveloper = 1 << 22,
        ApplicationCommandBadge = 1 << 23,
        Quarantined = 1ULL << 44,

        MaxFlag_PlusOne,
        MaxFlag = MaxFlag_PlusOne - 1,
    };

    static const char *GetFlagName(uint64_t flag);
    static const char *GetFlagReadableName(uint64_t flag);

    Snowflake ID;
    std::string Username;
    std::string Discriminator;
    std::string Avatar; // null
    std::optional<std::string> GlobalName;
    std::optional<bool> IsBot;
    std::optional<bool> IsSystem;
    std::optional<bool> IsMFAEnabled;
    std::optional<std::string> Locale;
    std::optional<bool> IsVerified;
    std::optional<std::string> Email; // null
    std::optional<uint64_t> Flags;
    std::optional<EPremiumType> PremiumType; // null
    std::optional<uint64_t> PublicFlags;

    // undocumented (opt)
    std::optional<bool> IsDesktop;
    std::optional<bool> IsMobile;
    std::optional<bool> IsNSFWAllowed; // null
    std::optional<std::string> Phone;  // null?
    // for now (unserialized)
    std::optional<std::string> BannerHash; // null
    std::optional<std::string> Bio;        // null

    friend void from_json(const nlohmann::json &j, UserData &m);
    friend void to_json(nlohmann::json &j, const UserData &m);
    void update_from_json(const nlohmann::json &j);

    [[nodiscard]] bool IsPomelo() const noexcept;
    [[nodiscard]] bool IsABot() const noexcept;
    [[nodiscard]] bool IsDeleted() const;
    [[nodiscard]] bool HasAvatar() const;
    [[nodiscard]] bool HasAnimatedAvatar() const noexcept;
    [[nodiscard]] bool HasAnimatedAvatar(Snowflake guild_id) const;
    [[nodiscard]] bool HasAnimatedAvatar(const std::optional<Snowflake> &guild_id) const;
    [[nodiscard]] std::string GetAvatarURL(Snowflake guild_id, const std::string &ext = "png", std::string size = "32") const;
    [[nodiscard]] std::string GetAvatarURL(const std::optional<Snowflake> &guild_id, const std::string &ext = "png", std::string size = "32") const;
    [[nodiscard]] std::string GetAvatarURL(const std::string &ext = "png", std::string size = "32") const;
    [[nodiscard]] std::string GetDefaultAvatarURL() const;
    [[nodiscard]] Snowflake GetHoistedRole(Snowflake guild_id, bool with_color = false) const;
    [[nodiscard]] std::string GetMention() const;
    [[nodiscard]] std::string GetDisplayName() const;
    [[nodiscard]] std::string GetDisplayName(Snowflake guild_id) const;
    [[nodiscard]] std::string GetDisplayName(const std::optional<Snowflake> &guild_id) const;
    [[nodiscard]] std::string GetDisplayNameEscaped() const;
    [[nodiscard]] std::string GetDisplayNameEscaped(Snowflake guild_id) const;
    [[nodiscard]] std::string GetDisplayNameEscapedBold() const;
    [[nodiscard]] std::string GetDisplayNameEscapedBold(Snowflake guild_id) const;
    [[nodiscard]] std::string GetUsername() const;
    [[nodiscard]] std::string GetUsernameEscaped() const;
    [[nodiscard]] std::string GetUsernameEscapedBold() const;
    [[nodiscard]] std::string GetUsernameEscapedBoldAt() const;
};
