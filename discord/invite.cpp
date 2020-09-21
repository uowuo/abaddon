#include "invite.hpp"

void from_json(const nlohmann::json &j, Invite &m) {
    JS_D("code", m.Code);
    JS_O("channel", m.Channel);
    JS_O("inviter", m.Inviter);
    JS_O("approximate_member_count", m.Members);

    if (j.contains("guild")) {
        auto x = j.at("guild");
        x.at("id").get_to(m.Guild.ID);
        x.at("name").get_to(m.Guild.Name);
    }
}
