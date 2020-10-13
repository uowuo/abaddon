#include "member.hpp"

void from_json(const nlohmann::json &j, GuildMember &m) {
    JS_O("user", m.User);
    JS_ON("nick", m.Nickname);
    JS_D("roles", m.Roles);
    JS_D("joined_at", m.JoinedAt);
    JS_ON("premium_since", m.PremiumSince);
    JS_D("deaf", m.IsDeafened);
    JS_D("mute", m.IsMuted);
}

GuildMember GuildMember::from_update_json(const nlohmann::json &j) {
    GuildMember ret;
    JS_D("roles", ret.Roles);
    JS_D("user", ret.User);
    JS_ON("nick", ret.Nickname);
    JS_D("joined_at", ret.JoinedAt);
    JS_ON("premium_since", ret.PremiumSince);
    return ret;
}
