#pragma once
#include "json.hpp"
#include "guild.hpp"
#include <string>

class Invite {
public:
    std::string Code;    //
    GuildData Guild;     // opt
    ChannelData Channel; // opt
    UserData Inviter;    // opt
    int Members = -1;    // opt

    friend void from_json(const nlohmann::json &j, Invite &m);
};
