#include <vector>
#include <nlohmann/json.hpp>
#include "discord/snowflake.hpp"

struct ExpansionState;
struct ExpansionStateRoot {
    std::map<Snowflake, ExpansionState> Children;

    friend void to_json(nlohmann::json &j, const ExpansionStateRoot &m);
    friend void from_json(const nlohmann::json &j, ExpansionStateRoot &m);
};

struct ExpansionState {
    bool IsExpanded;
    ExpansionStateRoot Children;

    friend void to_json(nlohmann::json &j, const ExpansionState &m);
    friend void from_json(const nlohmann::json &j, ExpansionState &m);
};

struct AbaddonApplicationState {
    Snowflake ActiveChannel;
    ExpansionStateRoot Expansion;

    friend void to_json(nlohmann::json &j, const AbaddonApplicationState &m);
    friend void from_json(const nlohmann::json &j, AbaddonApplicationState &m);
};
