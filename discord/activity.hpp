#pragma once
#include <string>
#include <optional>
#include "../util.hpp"
#include "json.hpp"
#include "snowflake.hpp"

enum class ActivityType : int {
    Game = 0,
    Streaming = 1,
    Listening = 2,
    Watching = 3, // not documented
    Custom = 4,
    Competing = 5,
};

enum class ActivityFlags {
    INSTANCE = (1 << 0),
    JOIN = (1 << 1),
    SPECTATE = (1 << 2),
    JOIN_REQUEST = (1 << 3),
    SYNC = (1 << 4),
    PLAY = (1 << 5),
};
template<>
struct Bitwise<ActivityFlags> {
    static const bool enable = true;
};

struct ActivityTimestamps {
    std::optional<std::string> Start; // opt
    std::optional<std::string> End;   // opt

    friend void from_json(const nlohmann::json &j, ActivityTimestamps &m);
    friend void to_json(nlohmann::json &j, const ActivityTimestamps &m);
};

struct ActivityEmoji {
    std::string Name;
    std::optional<Snowflake> ID;
    std::optional<bool> IsAnimated;

    friend void from_json(const nlohmann::json &j, ActivityEmoji &m);
    friend void to_json(nlohmann::json &j, const ActivityEmoji &m);
};

struct ActivityParty {
    std::optional<std::string> ID;
    std::optional<std::array<int, 2>> Size;

    friend void from_json(const nlohmann::json &j, ActivityParty &m);
    friend void to_json(nlohmann::json &j, const ActivityParty &m);
};

struct ActivityAssets {
    std::optional<std::string> LargeImage;
    std::optional<std::string> LargeText;
    std::optional<std::string> SmallImage;
    std::optional<std::string> SmallText;

    friend void from_json(const nlohmann::json &j, ActivityAssets &m);
    friend void to_json(nlohmann::json &j, const ActivityAssets &m);
};

struct ActivitySecrets {
    std::optional<std::string> Join;
    std::optional<std::string> Spectate;
    std::optional<std::string> Match;

    friend void from_json(const nlohmann::json &j, ActivitySecrets &m);
    friend void to_json(nlohmann::json &j, const ActivitySecrets &m);
};

struct Activity {
    std::string Name;                             //
    ActivityType Type;                            //
    std::optional<std::string> URL;               // null
    std::optional<uint64_t> CreatedAt;            //
    std::optional<ActivityTimestamps> Timestamps; //
    std::optional<Snowflake> ApplicationID;       //
    std::optional<std::string> Details;           // null
    std::optional<std::string> State;             // null
    std::optional<ActivityEmoji> Emoji;           // null
    std::optional<ActivityParty> Party;           //
    std::optional<ActivityAssets> Assets;         //
    std::optional<ActivitySecrets> Secrets;       //
    std::optional<bool> IsInstance;               //
    std::optional<ActivityFlags> Flags;           //

    friend void from_json(const nlohmann::json &j, Activity &m);
    friend void to_json(nlohmann::json &j, const Activity &m);
};
