#include "usersettings.hpp"

void from_json(const nlohmann::json &j, UserSettingsGuildFoldersEntry &m) {
    JS_N("color", m.Color);
    JS_D("guild_ids", m.GuildIDs);
    JS_N("id", m.ID);
    JS_N("name", m.Name);
}

void from_json(const nlohmann::json &j, UserSettings &m) {
    JS_D("guild_folders", m.GuildFolders);
}
