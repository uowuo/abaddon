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

std::optional<PermissionOverwrite> Channel::GetOverwrite(Snowflake id) const {
    auto ret = std::find_if(PermissionOverwrites.begin(), PermissionOverwrites.end(), [id](const auto x) { return x.ID == id; });
    if (ret != PermissionOverwrites.end())
        return *ret;
    return std::nullopt;
}
