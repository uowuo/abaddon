#pragma once
#include <string>
#include <optional>
#include "member.hpp"
#include "user.hpp"
#include "snowflake.hpp"

enum class InteractionType {
    Pong = 1,                             // ACK a Ping
    Acknowledge = 2,                      // DEPRECATED ACK a command without sending a message, eating the user's input
    ChannelMessage = 3,                   // DEPRECATED respond with a message, eating the user's input
    ChannelMessageWithSource = 4,         // respond to an interaction with a message
    DeferredChannelMessageWithSource = 5, // ACK an interaction and edit to a response later, the user sees a loading state
};

struct MessageInteractionData {
    Snowflake ID;         // id of the interaction
    InteractionType Type; // the type of interaction
    std::string Name;     // the name of the ApplicationCommand
    UserData User;        // the user who invoked the interaction
    // undocumented???
    std::optional<GuildMember> Member; // the member who invoked the interaction (in a guild)

    friend void from_json(const nlohmann::json &j, MessageInteractionData &m);
};
