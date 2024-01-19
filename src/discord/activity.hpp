#pragma once
#include <string>
#include <optional>
#include "json.hpp"
#include "snowflake.hpp"
#include "misc/bitwise.hpp"

enum class PresenceStatus : uint8_t {
    Online,
    Offline,
    Idle,
    DND,
};

constexpr inline const char *GetPresenceString(PresenceStatus s) {
    switch (s) {
        case PresenceStatus::Online:
            return "online";
        case PresenceStatus::Offline:
            return "offline";
        case PresenceStatus::Idle:
            return "idle";
        case PresenceStatus::DND:
            return "dnd";
    }
    return "";
}

constexpr inline const char *GetPresenceDisplayString(PresenceStatus s) {
    switch (s) {
        case PresenceStatus::Online:
            return "Online";
        case PresenceStatus::Offline:
            return "Offline";
        case PresenceStatus::Idle:
            return "Away";
        case PresenceStatus::DND:
            return "Do Not Disturb";
    }
    return "";
}

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
    std::optional<int> Start;
    std::optional<int> End;

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

struct ActivityData {
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

    friend void from_json(const nlohmann::json &j, ActivityData &m);
    friend void to_json(nlohmann::json &j, const ActivityData &m);
};

struct PresenceData {
    std::vector<ActivityData> Activities; // null (but never sent as such)
    std::string Status;
    std::optional<bool> IsAFK;
    std::optional<int> Since;

    friend void from_json(const nlohmann::json &j, PresenceData &m);
    friend void to_json(nlohmann::json &j, const PresenceData &m);
};
