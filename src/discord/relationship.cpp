#include "relationship.hpp"

void from_json(const nlohmann::json &j, RelationshipData &m) {
    JS_D("type", m.Type);
    JS_D("id", m.ID);
}
