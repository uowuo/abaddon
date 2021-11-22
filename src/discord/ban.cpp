#include "ban.hpp"

void from_json(const nlohmann::json &j, BanData &m) {
    JS_N("reason", m.Reason);
    JS_D("user", m.User);
}
