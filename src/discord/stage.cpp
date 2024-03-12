#include "stage.hpp"

#include "json.hpp"

void from_json(const nlohmann::json &j, StageInstance &m) {
    JS_D("id", m.ID);
    JS_D("guild_id", m.GuildID);
    JS_D("channel_id", m.ChannelID);
    JS_N("topic", m.Topic);
    JS_N("privacy_level", m.PrivacyLevel);
    JS_N("guild_scheduled_event_id", m.GuildScheduledEventID);
}
