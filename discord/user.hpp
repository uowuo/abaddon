#pragma once
#include "snowflake.hpp"
#include "json.hpp"
#include <string>

struct User {
    Snowflake ID;              //
    std::string Username;      //
    std::string Discriminator; //
    std::string Avatar;        // null
    bool IsBot = false;        // opt
    bool IsSystem = false;     // opt
    bool IsMFAEnabled = false; // opt
    std::string Locale;        // opt
    bool IsVerified = false;   // opt
    std::string Email;         // opt, null
    int Flags = 0;             // opt
    int PremiumType = 0;       // opt, null (docs wrong)
    int PublicFlags = 0;       // opt

    // undocumented (opt)
    bool IsDesktop = false;     //
    bool IsMobile = false;      //
    bool IsNSFWAllowed = false; // null
    std::string Phone;          // null?

    friend void from_json(const nlohmann::json &j, User &m);

    bool HasAvatar() const;
    std::string GetAvatarURL(std::string ext = "png", std::string size = "64") const;
};
