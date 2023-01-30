#include "interactions.hpp"
#include "json.hpp"

void from_json(const nlohmann::json &j, MessageInteractionData &m) {
    JS_D("id", m.ID);
    JS_D("type", m.Type);
    JS_D("name", m.Name);
    JS_D("user", m.User);
    JS_O("member", m.Member);
}
