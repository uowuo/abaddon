#pragma once
#include <string>
#include <vector>
#include "json.hpp"
#include "snowflake.hpp"
#include "user.hpp"

struct EmojiData {
    Snowflake ID;     // null
    std::string Name; // null (in reactions)
    std::optional<std::vector<Snowflake>> Roles;
    std::optional<UserData> Creator; // only reliable to access ID
    std::optional<bool> NeedsColons;
    std::optional<bool> IsManaged;
    std::optional<bool> IsAnimated;
    std::optional<bool> IsAvailable;

    friend void from_json(const nlohmann::json &j, EmojiData &m);
    friend void to_json(nlohmann::json &j, const EmojiData &m);

    std::string GetURL(const char *ext = "png", const char *size = nullptr) const;
    static std::string URLFromID(const std::string &emoji_id, const char *ext = "png", const char *size = nullptr);
    static std::string URLFromID(Snowflake emoji_id, const char *ext = "png", const char *size = nullptr);
    static std::string URLFromID(const Glib::ustring &emoji_id, const char *ext = "png", const char *size = nullptr);

    bool IsEmojiAnimated() const noexcept;
};
