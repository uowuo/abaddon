#pragma once
#include "json.hpp"
#include "snowflake.hpp"
#include <optional>
#include <string>

struct UserSettingsGuildFoldersEntry {
    std::optional<int> Color;
    std::vector<Snowflake> GuildIDs;
    std::optional<Snowflake> ID; // (this can be a snowflake as a string or an int that isnt a snowflake lol)
    std::optional<std::string> Name;

    friend void from_json(const nlohmann::json &j, UserSettingsGuildFoldersEntry &m);
};

struct UserSettings {
    std::vector<UserSettingsGuildFoldersEntry> GuildFolders;
    /*
    int TimezoneOffset;
    std::string Theme;
    bool AreStreamNotificationsEnabled;
    std::string Status;
    bool ShouldShowCurrentGame;
    // std::vector<Unknown> RestrictedGuilds;
    bool ShouldRenderReactions;
    bool ShouldRenderEmbeds;
    bool IsNativePhoneIntegrationEnabled;
    bool ShouldMessageDisplayCompact;
    std::string Locale;
    bool ShouldInlineEmbedMedia;
    bool ShouldInlineAttachmentMedia;
    std::vector<Snowflake> GuildPositions; // deprecated?
    bool ShouldGIFAutoplay;
    // Unknown FriendSourceFlags;
    int ExplicitContentFilter;
    bool IsTTSCommandEnabled;
    bool ShouldDisableGamesTab;
    bool DeveloperMode;
    bool ShouldDetectPlatformAccounts;
    bool AreDefaultGuildsRestricted;
    // Unknown CustomStatus; // null
    bool ShouldConvertEmoticons;
    bool IsContactSyncEnabled;
    bool ShouldAnimateEmojis;
    bool IsAccessibilityDetectionAllowed;
    int AFKTimeout;*/

    friend void from_json(const nlohmann::json &j, UserSettings &m);
};
