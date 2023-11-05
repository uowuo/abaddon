#include "emoji.hpp"

void from_json(const nlohmann::json &j, EmojiData &m) {
    JS_N("id", m.ID);
    JS_N("name", m.Name);
    JS_O("roles", m.Roles);
    JS_O("user", m.Creator);
    JS_O("require_colons", m.NeedsColons);
    JS_O("managed", m.IsManaged);
    JS_O("animated", m.IsAnimated);
    JS_O("available", m.IsAvailable);
}

void to_json(nlohmann::json &j, const EmojiData &m) {
    if (m.ID.IsValid())
        j["id"] = m.ID;
    else
        j["id"] = nullptr;
    if (!m.Name.empty())
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

std::string EmojiData::GetURL(const char *ext, const char *size) const {
    if (size != nullptr)
        return "https://cdn.discordapp.com/emojis/" + std::to_string(ID) + "." + ext + "?size=" + size;
    else
        return "https://cdn.discordapp.com/emojis/" + std::to_string(ID) + "." + ext;
}

std::string EmojiData::URLFromID(const std::string &emoji_id, const char *ext, const char *size) {
    if (size != nullptr)
        return "https://cdn.discordapp.com/emojis/" + emoji_id + "." + ext + "?size=" + size;
    else
        return "https://cdn.discordapp.com/emojis/" + emoji_id + "." + ext;
}

std::string EmojiData::URLFromID(Snowflake emoji_id, const char *ext, const char *size) {
    return URLFromID(std::to_string(emoji_id), ext, size);
}

std::string EmojiData::URLFromID(const Glib::ustring &emoji_id, const char *ext, const char *size) {
    return URLFromID(emoji_id.raw(), ext, size);
}

bool EmojiData::IsEmojiAnimated() const noexcept {
    return IsAnimated.has_value() && *IsAnimated;
}
