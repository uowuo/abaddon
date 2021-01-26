#include "activity.hpp"

void from_json(const nlohmann::json &j, ActivityTimestamps &m) {
    JS_O("start", m.Start);
    JS_O("end", m.End);
}

void to_json(nlohmann::json &j, const ActivityTimestamps &m) {
    JS_IF("start", m.Start);
    JS_IF("end", m.End);
}

void from_json(const nlohmann::json &j, ActivityEmoji &m) {
    JS_D("name", m.Name);
    JS_O("id", m.ID);
    JS_O("animated", m.IsAnimated);
}

void to_json(nlohmann::json &j, const ActivityEmoji &m) {
    j["name"] = m.Name;
    if (m.ID.has_value())
        j["id"] = *m.ID;
    if (m.IsAnimated.has_value())
        j["animated"] = *m.IsAnimated;
}

void from_json(const nlohmann::json &j, ActivityParty &m) {
    JS_O("id", m.ID);
    JS_O("size", m.Size);
}

void to_json(nlohmann::json &j, const ActivityParty &m) {
    JS_IF("id", m.ID);
    JS_IF("size", m.Size);
}

void from_json(const nlohmann::json &j, ActivityAssets &m) {
    JS_O("large_image", m.LargeImage);
    JS_O("large_text", m.LargeText);
    JS_O("small_image", m.SmallImage);
    JS_O("small_text", m.SmallText);
}

void to_json(nlohmann::json &j, const ActivityAssets &m) {
    JS_IF("large_image", m.LargeImage);
    JS_IF("large_text", m.LargeText);
    JS_IF("small_image", m.SmallImage);
    JS_IF("small_text", m.SmallText);
}

void from_json(const nlohmann::json &j, ActivitySecrets &m) {
    JS_O("join", m.Join);
    JS_O("spectate", m.Spectate);
    JS_O("match", m.Match);
}

void to_json(nlohmann::json &j, const ActivitySecrets &m) {
    JS_IF("join", m.Join);
    JS_IF("spectate", m.Spectate);
    JS_IF("match", m.Match);
}

void from_json(const nlohmann::json &j, ActivityData &m) {
    JS_D("name", m.Name);
    JS_D("type", m.Type);
    JS_ON("url", m.URL);
    JS_D("created_at", m.CreatedAt);
    JS_O("timestamps", m.Timestamps);
    JS_O("application_id", m.ApplicationID);
    JS_ON("details", m.Details);
    JS_ON("state", m.State);
    JS_ON("emoji", m.Emoji);
    JS_ON("party", m.Party);
    JS_O("assets", m.Assets);
    JS_O("secrets", m.Secrets);
    JS_O("instance", m.IsInstance);
    JS_O("flags", m.Flags);
}

void to_json(nlohmann::json &j, const ActivityData &m) {
    if (m.Type == ActivityType::Custom) {
        j["name"] = "Custom Status";
        j["state"] = m.Name;
    } else {
        j["name"] = m.Name;
        JS_IF("state", m.State);
    }

    j["type"] = m.Type;
    JS_IF("url", m.URL);
    JS_IF("created_at", m.CreatedAt);
    JS_IF("timestamps", m.Timestamps);
    JS_IF("application_id", m.ApplicationID);
    JS_IF("details", m.Details);
    JS_IF("emoji", m.Emoji);
    JS_IF("party", m.Party);
    JS_IF("assets", m.Assets);
    JS_IF("secrets", m.Secrets);
    JS_IF("instance", m.IsInstance);
    JS_IF("flags", m.Flags);
}

void from_json(const nlohmann::json &j, PresenceData &m) {
    JS_N("activities", m.Activities);
    JS_D("status", m.Status);
}

void to_json(nlohmann::json &j, const PresenceData &m) {
    j["activities"] = m.Activities;
    j["status"] = m.Status;
    JS_IF("afk", m.IsAFK);
    if (m.Since.has_value())
        j["since"] = *m.Since;
    else
        j["since"] = 0;
}
