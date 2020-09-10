#include "usersettings.hpp"

void from_json(const nlohmann::json &j, UserSettings &m) {
    JS_D("timezone_offset", m.TimezoneOffset);
    JS_D("theme", m.Theme);
    JS_D("stream_notifications_enabled", m.AreStreamNotificationsEnabled);
    JS_D("status", m.Status);
    JS_D("show_current_game", m.ShouldShowCurrentGame);
    // JS_D("restricted_guilds", m.RestrictedGuilds);
    JS_D("render_reactions", m.ShouldRenderReactions);
    JS_D("render_embeds", m.ShouldRenderEmbeds);
    JS_D("native_phone_integration_enabled", m.IsNativePhoneIntegrationEnabled);
    JS_D("message_display_compact", m.ShouldMessageDisplayCompact);
    JS_D("locale", m.Locale);
    JS_D("inline_embed_media", m.ShouldInlineEmbedMedia);
    JS_D("inline_attachment_media", m.ShouldInlineAttachmentMedia);
    JS_D("guild_positions", m.GuildPositions);
    // JS_D("guild_folders", m.GuildFolders);
    JS_D("gif_auto_play", m.ShouldGIFAutoplay);
    // JS_D("friend_source_flags", m.FriendSourceFlags);
    JS_D("explicit_content_filter", m.ExplicitContentFilter);
    JS_D("enable_tts_command", m.IsTTSCommandEnabled);
    JS_D("disable_games_tab", m.ShouldDisableGamesTab);
    JS_D("developer_mode", m.DeveloperMode);
    JS_D("detect_platform_accounts", m.ShouldDetectPlatformAccounts);
    JS_D("default_guilds_restricted", m.AreDefaultGuildsRestricted);
    // JS_N("custom_status", m.CustomStatus);
    JS_D("convert_emoticons", m.ShouldConvertEmoticons);
    JS_D("contact_sync_enabled", m.IsContactSyncEnabled);
    JS_D("animate_emoji", m.ShouldAnimateEmojis);
    JS_D("allow_accessibility_detection", m.IsAccessibilityDetectionAllowed);
    JS_D("afk_timeout", m.AFKTimeout);
}
