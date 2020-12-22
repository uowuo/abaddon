#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include <string>

struct User {
    Snowflake ID;
    std::string Username;
    std::string Discriminator;
    std::string Avatar; // null
    std::optional<bool> IsBot;
    std::optional<bool> IsSystem;
    std::optional<bool> IsMFAEnabled;
    std::optional<std::string> Locale;
    std::optional<bool> IsVerified;
    std::optional<std::string> Email; // null
    std::optional<int> Flags;
    std::optional<int> PremiumType; // null
    std::optional<int> PublicFlags;

    // undocumented (opt)
    bool IsDesktop = false;     //
    bool IsMobile = false;      //
    bool IsNSFWAllowed = false; // null
    std::string Phone;          // null?

    friend void from_json(const nlohmann::json &j, User &m);
    friend void to_json(nlohmann::json &j, const User &m);
    static void update_from_json(const nlohmann::json &j, User &m);

    bool HasAvatar() const;
    bool HasAnimatedAvatar() const;
    std::string GetAvatarURL(std::string ext = "png", std::string size = "32") const;
    Snowflake GetHoistedRole(Snowflake guild_id, bool with_color = false) const;
    std::string GetMention() const;
};
