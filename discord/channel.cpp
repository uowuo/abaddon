#include "../abaddon.hpp"
#include "channel.hpp"

void from_json(const nlohmann::json &j, Channel &m) {
    JS_D("id", m.ID);
    JS_D("type", m.Type);
    JS_O("guild_id", m.GuildID);
    JS_O("position", m.Position);
    JS_O("permission_overwrites", m.PermissionOverwrites);
    JS_ON("name", m.Name);
    JS_ON("topic", m.Topic);
    JS_O("nsfw", m.IsNSFW);
    JS_ON("last_message_id", m.LastMessageID);
    JS_O("bitrate", m.Bitrate);
    JS_O("user_limit", m.UserLimit);
    JS_O("rate_limit_per_user", m.RateLimitPerUser);
    JS_O("recipients", m.Recipients);
    JS_ON("icon", m.Icon);
    JS_O("owner_id", m.OwnerID);
    JS_O("application_id", m.ApplicationID);
    JS_ON("parent_id", m.ParentID);
    JS_ON("last_pin_timestamp", m.LastPinTimestamp);
}

void Channel::update_from_json(const nlohmann::json &j) {
    JS_RD("type", Type);
    JS_RD("guild_id", GuildID);
    JS_RV("position", Position, -1);
    JS_RD("permission_overwrites", PermissionOverwrites);
    JS_RD("name", Name);
    JS_RD("topic", Topic);
    JS_RD("nsfw", IsNSFW);
    JS_RD("last_message_id", LastMessageID);
    JS_RD("bitrate", Bitrate);
    JS_RD("user_limit", UserLimit);
    JS_RD("rate_limit_per_user", RateLimitPerUser);
    JS_RD("recipients", Recipients);
    JS_RD("icon", Icon);
    JS_RD("owner_id", OwnerID);
    JS_RD("application_id", ApplicationID);
    JS_RD("parent_id", ParentID);
    JS_RD("last_pin_timestamp", LastPinTimestamp);
}

std::optional<PermissionOverwrite> Channel::GetOverwrite(Snowflake id) const {
    return Abaddon::Get().GetDiscordClient().GetPermissionOverwrite(ID, id);
}
