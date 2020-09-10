#include "user.hpp"

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
