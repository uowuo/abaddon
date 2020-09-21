#pragma once
#include "json.hpp"
#include "guild.hpp"
#include <string>

class Invite {
public:
    std::string Code; //
    Guild Guild;      // opt
    Channel Channel;  // opt
    User Inviter;     // opt
    int Members = -1; // opt

    friend void from_json(const nlohmann::json &j, Invite &m);
};
