#include "emoji.hpp"

void from_json(const nlohmann::json &j, Emoji &m) {
    JS_N("id", m.ID);
    JS_N("name", m.Name);
    JS_O("roles", m.Roles);
    JS_O("user", m.Creator);
    JS_O("require_colons", m.NeedsColons);
    JS_O("managed", m.IsManaged);
    JS_O("animated", m.IsAnimated);
    JS_O("available", m.IsAvailable);
}

void to_json(nlohmann::json &j, const Emoji &m) {
    if (m.ID.IsValid())
        j["id"] = m.ID;
    else
        j["id"] = nullptr;
    if (m.Name != "")
        j["name"] = m.Name;
    else
        j["name"] = nullptr;
    JS_IF("roles", m.Roles);
    JS_IF("user", m.Creator);
    JS_IF("require_colons", m.NeedsColons);
    JS_IF("managed", m.IsManaged);
    JS_IF("animated", m.IsAnimated);
    JS_IF("available", m.IsAvailable);
}

std::string Emoji::GetURL() const {
    return "https://cdn.discordapp.com/emojis/" + std::to_string(ID) + ".png";
}

std::string Emoji::URLFromID(std::string emoji_id) {
    return "https://cdn.discordapp.com/emojis/" + emoji_id + ".png";
}
