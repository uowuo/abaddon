#include "permissions.hpp"

void from_json(const nlohmann::json &j, PermissionOverwrite &m) {
    JS_D("id", m.ID);
    std::string tmp;
    JS_D("type", tmp);
    m.ID = tmp == "role" ? PermissionOverwrite::ROLE : PermissionOverwrite::MEMBER;
    JS_D("allow_new", tmp);
    m.Allow = static_cast<Permission>(std::stoull(tmp));
    JS_D("deny_new", tmp);
    m.Deny = static_cast<Permission>(std::stoull(tmp));
}
