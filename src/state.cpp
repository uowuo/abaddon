#include "state.hpp"

void to_json(nlohmann::json &j, const ExpansionStateRoot &m) {
    if (m.Children.empty()) {
        j = nlohmann::json::object();
    } else {
        for (const auto &[id, state] : m.Children)
            j[std::to_string(id)] = state;
    }
}

void from_json(const nlohmann::json &j, ExpansionStateRoot &m) {
    for (const auto &[key, value] : j.items())
        m.Children[key] = value;
}

void to_json(nlohmann::json &j, const ExpansionState &m) {
    j["e"] = m.IsExpanded;
    j["c"] = m.Children;
}

void from_json(const nlohmann::json &j, ExpansionState &m) {
    j.at("e").get_to(m.IsExpanded);
    j.at("c").get_to(m.Children);
}

void to_json(nlohmann::json &j, const TabsState &m) {
    j = m.Channels;
}

void from_json(const nlohmann::json &j, TabsState &m) {
    j.get_to(m.Channels);
}

void to_json(nlohmann::json &j, const AbaddonApplicationState &m) {
    j["active_channel"] = m.ActiveChannel;
    j["expansion"] = m.Expansion;
    j["tabs"] = m.Tabs;
}

void from_json(const nlohmann::json &j, AbaddonApplicationState &m) {
    if (j.contains("active_channel"))
        j.at("active_channel").get_to(m.ActiveChannel);
    if (j.contains("expansion"))
        j.at("expansion").get_to(m.Expansion);
    if (j.contains("tabs"))
        j.at("tabs").get_to(m.Tabs);
}
