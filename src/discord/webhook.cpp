#include "webhook.hpp"

void from_json(const nlohmann::json &j, WebhookData &m) {
    JS_D("id", m.ID);
    JS_D("type", m.Type);
    JS_O("guild_id", m.GuildID);
    JS_D("channel_id", m.ChannelID);
    JS_O("user", m.User);
    JS_N("name", m.Name);
    JS_N("avatar", m.Avatar);
    JS_O("token", m.Token);
    JS_N("application_id", m.ApplicationID);
}

std::string WebhookMessageData::GetAvatarURL() const {
    return Avatar.empty() ? "" : "https://cdn.discordapp.com/avatars/" + std::to_string(WebhookID) + "/" + Avatar + ".png?size=32";
}
