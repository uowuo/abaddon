#include "user.hpp"
#include "../abaddon.hpp"

bool User::HasAvatar() const {
    return Avatar.size() > 0;
}

std::string User::GetAvatarURL(std::string ext, std::string size) const {
    return "https://cdn.discordapp.com/avatars/" + std::to_string(ID) + "/" + Avatar + "." + ext + "?size=" + size;
}

Snowflake User::GetHoistedRole(Snowflake guild_id, bool with_color) const {
    return Abaddon::Get().GetDiscordClient().GetMemberHoistedRole(guild_id, ID, with_color);
}

void from_json(const nlohmann::json &j, User &m) {
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
}

void to_json(nlohmann::json &j, const User &m) {
    j["id"] = m.ID;
    j["username"] = m.Username;
    j["avatar"] = m.Avatar;
    // rest of stuff as needed im too lazy and its probably not necessary
}

void User::update_from_json(const nlohmann::json &j, User &m) {
    JS_RD("username", m.Username);
    JS_RD("discriminator", m.Discriminator);
    JS_RD("avatar", m.Avatar);
    JS_RD("bot", m.IsBot);
    JS_RD("system", m.IsSystem);
    JS_RD("mfa_enabled", m.IsMFAEnabled);
    JS_RD("locale", m.Locale);
    JS_RD("verified", m.IsVerified);
    JS_RD("email", m.Email);
    JS_RD("flags", m.Flags);
    JS_RD("premium_type", m.PremiumType);
    JS_RD("public_flags", m.PublicFlags);
    JS_RD("desktop", m.IsDesktop);
    JS_RD("mobile", m.IsMobile);
    JS_RD("nsfw_allowed", m.IsNSFWAllowed);
    JS_RD("phone", m.Phone);
}
