#include "user.hpp"
#include "abaddon.hpp"

bool UserData::IsDeleted() const {
    return Discriminator == "0000";
}

bool UserData::HasAvatar() const {
    return Avatar.size() > 0;
}

bool UserData::HasAnimatedAvatar() const {
    return Avatar.size() > 0 && Avatar[0] == 'a' && Avatar[1] == '_';
}

std::string UserData::GetAvatarURL(Snowflake guild_id, std::string ext, std::string size) const {
    const auto member = Abaddon::Get().GetDiscordClient().GetMember(ID, guild_id);
    if (member.has_value() && member->Avatar.has_value())
        return "https://cdn.discordapp.com/guilds/" +
               std::to_string(guild_id) + "/users/" + std::to_string(ID) +
               "/avatars/" + *member->Avatar + "." +
               ext + "?" + "size=" + size;
    else
        return GetAvatarURL(ext, size);
}

std::string UserData::GetAvatarURL(const std::optional<Snowflake> &guild_id, std::string ext, std::string size) const {
    if (guild_id.has_value())
        return GetAvatarURL(*guild_id, ext, size);
    else
        return GetAvatarURL(ext, size);
}

std::string UserData::GetAvatarURL(std::string ext, std::string size) const {
    if (HasAvatar())
        return "https://cdn.discordapp.com/avatars/" + std::to_string(ID) + "/" + Avatar + "." + ext + "?size=" + size;
    else
        return GetDefaultAvatarURL();
}

std::string UserData::GetDefaultAvatarURL() const {
    return "https://cdn.discordapp.com/embed/avatars/" + std::to_string(std::stoul(Discriminator) % 5) + ".png"; // size isn't respected by the cdn
}

Snowflake UserData::GetHoistedRole(Snowflake guild_id, bool with_color) const {
    return Abaddon::Get().GetDiscordClient().GetMemberHoistedRole(guild_id, ID, with_color);
}

std::string UserData::GetMention() const {
    return "<@" + std::to_string(ID) + ">";
}

std::string UserData::GetEscapedName() const {
    return Glib::Markup::escape_text(Username);
}

std::string UserData::GetEscapedBoldName() const {
    return "<b>" + Glib::Markup::escape_text(Username) + "</b>";
}

std::string UserData::GetEscapedString() const {
    return Glib::Markup::escape_text(Username) + "#" + Discriminator;
}

void from_json(const nlohmann::json &j, UserData &m) {
    JS_D("id", m.ID);
    JS_D("username", m.Username);
    JS_D("discriminator", m.Discriminator);
    JS_N("avatar", m.Avatar);
    JS_O("bot", m.IsBot);
    JS_O("system", m.IsSystem);
    JS_O("mfa_enabled", m.IsMFAEnabled);
    JS_O("locale", m.Locale);
    JS_O("verified", m.IsVerified);
    JS_O("email", m.Email);
    JS_O("flags", m.Flags);
    JS_ON("premium_type", m.PremiumType);
    JS_O("public_flags", m.PublicFlags);
    JS_O("desktop", m.IsDesktop);
    JS_O("mobile", m.IsMobile);
    JS_ON("nsfw_allowed", m.IsNSFWAllowed);
    JS_ON("phone", m.Phone);
    JS_ON("bio", m.Bio);
    JS_ON("banner", m.BannerHash);
}

void to_json(nlohmann::json &j, const UserData &m) {
    j["id"] = m.ID;
    j["username"] = m.Username;
    j["discriminator"] = m.Discriminator;
    if (m.Avatar == "")
        j["avatar"] = nullptr;
    else
        j["avatar"] = m.Avatar;
    JS_IF("bot", m.IsBot);
    JS_IF("system", m.IsSystem);
    JS_IF("mfa_enabled", m.IsMFAEnabled);
    JS_IF("locale", m.Locale);
    JS_IF("verified", m.IsVerified);
    JS_IF("email", m.Email);
    JS_IF("flags", m.Flags);
    JS_IF("premium_type", m.PremiumType);
    JS_IF("public_flags", m.PublicFlags);
    JS_IF("desktop", m.IsDesktop);
    JS_IF("mobile", m.IsMobile);
    JS_IF("nsfw_allowed", m.IsNSFWAllowed);
    JS_IF("phone", m.Phone);
}

void UserData::update_from_json(const nlohmann::json &j) {
    JS_RD("username", Username);
    JS_RD("discriminator", Discriminator);
    JS_RD("avatar", Avatar);
    JS_RD("bot", IsBot);
    JS_RD("system", IsSystem);
    JS_RD("mfa_enabled", IsMFAEnabled);
    JS_RD("locale", Locale);
    JS_RD("verified", IsVerified);
    JS_RD("email", Email);
    JS_RD("flags", Flags);
    JS_RD("premium_type", PremiumType);
    JS_RD("public_flags", PublicFlags);
    JS_RD("desktop", IsDesktop);
    JS_RD("mobile", IsMobile);
    JS_RD("nsfw_allowed", IsNSFWAllowed);
    JS_RD("phone", Phone);
}

const char *UserData::GetFlagName(uint64_t flag) {
    switch (flag) {
        case DiscordEmployee:
            return "discordstaff";
        case PartneredServerOwner:
            return "partneredowner";
        case HypeSquadEvents:
            return "hypesquadevents";
        case BugHunterLevel1:
            return "discordbughunter";
        case HouseBravery:
            return "hypesquadbravery";
        case HouseBrilliance:
            return "hypesquadbrilliance";
        case HouseBalance:
            return "hypesquadbalance";
        case EarlySupporter:
            return "earlysupporter";
        case TeamUser:
            return "teamuser";
        case System:
            return "system";
        case BugHunterLevel2:
            return "discordbughunter2";
        case VerifiedBot:
            return "verifiedbot";
        case EarlyVerifiedBotDeveloper:
            return "earlyverifiedbotdeveloper";
        case CertifiedModerator:
            return "certifiedmoderator";
        default:
            return "unknown";
    }
}

const char *UserData::GetFlagReadableName(uint64_t flag) {
    switch (flag) {
        case DiscordEmployee:
            return "Discord Staff";
        case PartneredServerOwner:
            return "Partnered Server Owner";
        case HypeSquadEvents:
            return "HypeSquad Events";
        case BugHunterLevel1:
            return "Discord Bug Hunter";
        case HouseBravery:
            return "HypeSquad Bravery";
        case HouseBrilliance:
            return "HypeSquad Brilliance";
        case HouseBalance:
            return "HypeSquad Balance";
        case EarlySupporter:
            return "Early Supporter";
        case TeamUser:
            return "Team User"; // ???
        case System:
            return "System";
        case BugHunterLevel2:
            return "Discord Bug Hunter Level 2";
        case VerifiedBot:
            return "Verified Bot";
        case EarlyVerifiedBotDeveloper:
            return "Early Verified Bot Developer";
        case CertifiedModerator:
            return "Discord Certified Moderator";
        default:
            return "";
    }
}
