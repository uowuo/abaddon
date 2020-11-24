#include "permissions.hpp"

void from_json(const nlohmann::json &j, PermissionOverwrite &m) {
    JS_D("id", m.ID);
    std::string tmp;
    m.Type = j.at("type").get<int>() == 0 ? PermissionOverwrite::ROLE : PermissionOverwrite::MEMBER;
    JS_D("allow", tmp);
    m.Allow = static_cast<Permission>(std::stoull(tmp));
    JS_D("deny", tmp);
    m.Deny = static_cast<Permission>(std::stoull(tmp));
}
