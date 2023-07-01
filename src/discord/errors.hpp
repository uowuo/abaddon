#pragma once

enum class DiscordError {
    GENERIC = 0,
    INVALID_FORM_BODY = 50035,
    RELATIONSHIP_INCOMING_DISABLED = 80000,
    RELATIONSHIP_INCOMING_BLOCKED = 80001,
    RELATIONSHIP_INVALID_USER_BOT = 80002, // this is misspelled in discord's source lul
    RELATIONSHIP_INVALID_SELF = 80003,
    RELATIONSHIP_INVALID_DISCORD_TAG = 80004,
    RELATIONSHIP_ALREADY_FRIENDS = 80007,

    NONE = -1,
    CAPTCHA_REQUIRED = -2,
};

constexpr const char *GetDiscordErrorDisplayString(DiscordError error) {
    switch (error) {
        case DiscordError::INVALID_FORM_BODY:
            return "Something's wrong with your input";
        case DiscordError::RELATIONSHIP_INCOMING_DISABLED:
            return "This user isn't accepting friend requests";
        case DiscordError::RELATIONSHIP_INCOMING_BLOCKED:
            return "You are blocked by this user";
        case DiscordError::RELATIONSHIP_INVALID_USER_BOT:
            return "You can't send a request to a bot";
        case DiscordError::RELATIONSHIP_INVALID_SELF:
            return "You can't send a request to yourself";
        case DiscordError::RELATIONSHIP_INVALID_DISCORD_TAG:
            return "No users with that tag exist";
        case DiscordError::RELATIONSHIP_ALREADY_FRIENDS:
            return "You are already friends with that user";
        case DiscordError::GENERIC:
        default:
            return "An error occurred";
    }
}
