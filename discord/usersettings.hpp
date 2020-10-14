#pragma once
#include "json.hpp"
#include "snowflake.hpp"
#include <string>

struct UserSettingsGuildFoldersEntry {
    int Color = -1; // null
    std::vector<Snowflake> GuildIDs;
    int ID = -1; // null
    std::string Name; // null

    friend void from_json(const nlohmann::json &j, UserSettingsGuildFoldersEntry &m);
};

struct UserSettings {
    int TimezoneOffset;                 //
    std::string Theme;                  //
    bool AreStreamNotificationsEnabled; //
    std::string Status;                 //
    bool ShouldShowCurrentGame;         //
    // std::vector<Unknown> RestrictedGuilds; //
    bool ShouldRenderReactions;            //
    bool ShouldRenderEmbeds;               //
    bool IsNativePhoneIntegrationEnabled;  //
    bool ShouldMessageDisplayCompact;      //
    std::string Locale;                    //
    bool ShouldInlineEmbedMedia;           //
    bool ShouldInlineAttachmentMedia;      //
    std::vector<Snowflake> GuildPositions; // deprecated?
    std::vector<UserSettingsGuildFoldersEntry> GuildFolders; //
    bool ShouldGIFAutoplay; //
    // Unknown FriendSourceFlags; //
    int ExplicitContentFilter;         //
    bool IsTTSCommandEnabled;          //
    bool ShouldDisableGamesTab;        //
    bool DeveloperMode;                //
    bool ShouldDetectPlatformAccounts; //
    bool AreDefaultGuildsRestricted;   //
    // Unknown CustomStatus; // null
    bool ShouldConvertEmoticons;          //
    bool IsContactSyncEnabled;            //
    bool ShouldAnimateEmojis;             //
    bool IsAccessibilityDetectionAllowed; //
    int AFKTimeout;

    friend void from_json(const nlohmann::json &j, UserSettings &m);
};
