#pragma once
#include <optional>
#include "json.hpp"
#include "snowflake.hpp"
#include "user.hpp"

enum class WebhookType {
    Incoming = 1,
    ChannelFollower = 2,
};

struct WebhookData {
    Snowflake ID;
    WebhookType Type;
    std::optional<Snowflake> GuildID;
    Snowflake ChannelID;
    std::optional<UserData> User;
    std::string Name;   // null
    std::string Avatar; // null
    std::optional<std::string> Token;
    Snowflake ApplicationID; // null

    friend void from_json(const nlohmann::json &j, WebhookData &m);
};

struct WebhookMessageData {
    Snowflake MessageID;
    Snowflake WebhookID;
    std::string Username;
    std::string Avatar;

    std::string GetAvatarURL() const;
};
