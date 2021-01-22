#include "invite.hpp"

void from_json(const nlohmann::json &j, InviteData &m) {
    JS_D("code", m.Code);
    JS_O("guild", m.Guild);
    JS_O("channel", m.Channel);
    JS_O("inviter", m.Inviter);
    JS_O("target_user", m.TargetUser);
    JS_O("target_user_type", m.TargetUserType);
    JS_O("approximate_presence_count", m.PresenceCount);
    JS_O("approximate_member_count", m.MemberCount);
    JS_O("uses", m.Uses);
    JS_O("max_uses", m.MaxUses);
    JS_O("max_age", m.MaxAge);
    JS_O("temporary", m.IsTemporary);
    JS_O("created_at", m.CreatedAt);
}
