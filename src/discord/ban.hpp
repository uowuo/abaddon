#pragma once
#include <string>
#include "user.hpp"

struct BanData {
    std::string Reason; // null
    UserData User;      // access id

    friend void from_json(const nlohmann::json &j, BanData &m);
};
