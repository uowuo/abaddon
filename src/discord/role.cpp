#include "role.hpp"

#include <glibmm/markup.h>

void from_json(const nlohmann::json &j, RoleData &m) {
    JS_D("id", m.ID);
    JS_D("name", m.Name);
    JS_D("color", m.Color);
    JS_D("hoist", m.IsHoisted);
    JS_D("position", m.Position);
    std::string tmp;
    JS_D("permissions", tmp);
    m.Permissions = static_cast<Permission>(std::stoull(tmp));
    JS_D("managed", m.IsManaged);
    JS_D("mentionable", m.IsMentionable);
}

bool RoleData::HasColor() const noexcept {
    return Color != 0;
}

Glib::ustring RoleData::GetEscapedName() const {
    return Glib::Markup::escape_text(Name);
}
