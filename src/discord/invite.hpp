#pragma once
#include "json.hpp"
#include "guild.hpp"
#include <string>

enum class ETargetUserType {
    STREAM = 1
};

class InviteChannelData {
public:
    InviteChannelData() = default;
    InviteChannelData(const ChannelData &c);

    Snowflake ID;
    ChannelType Type;
    std::optional<std::string> Name;
    std::optional<std::vector<std::string>> RecipientUsernames;
    // std::optional<??> Icon;

    friend void from_json(const nlohmann::json &j, InviteChannelData &m);
};

class InviteData {
public:
    std::string Code;
    std::optional<GuildData> Guild;
    std::optional<InviteChannelData> Channel;
    std::optional<UserData> Inviter;
    std::optional<UserData> TargetUser;
    std::optional<ETargetUserType> TargetUserType;
    std::optional<int> PresenceCount;
    std::optional<int> MemberCount;
    std::optional<int> Uses;
    std::optional<int> MaxUses;
    std::optional<int> MaxAge;
    std::optional<bool> IsTemporary;
    std::optional<std::string> CreatedAt;

    friend void from_json(const nlohmann::json &j, InviteData &m);
};
