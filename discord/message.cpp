#include "message.hpp"

void from_json(const nlohmann::json &j, EmbedFooterData &m) {
    JS_D("text", m.Text);
    JS_O("icon_url", m.IconURL);
    JS_O("proxy_icon_url", m.ProxyIconURL);
}

void from_json(const nlohmann::json &j, EmbedImageData &m) {
    JS_O("url", m.URL);
    JS_O("proxy_url", m.ProxyURL);
    JS_O("height", m.Height);
    JS_O("width", m.Width);
}

void from_json(const nlohmann::json &j, EmbedThumbnailData &m) {
    JS_O("url", m.URL);
    JS_O("proxy_url", m.ProxyURL);
    JS_O("height", m.Height);
    JS_O("width", m.Width);
}

void from_json(const nlohmann::json &j, EmbedVideoData &m) {
    JS_O("url", m.URL);
    JS_O("height", m.Height);
    JS_O("width", m.Width);
}

void from_json(const nlohmann::json &j, EmbedProviderData &m) {
    JS_O("name", m.Name);
    JS_ON("url", m.URL);
}

void from_json(const nlohmann::json &j, EmbedAuthorData &m) {
    JS_O("name", m.Name);
    JS_O("url", m.URL);
    JS_O("icon_url", m.IconURL);
    JS_O("proxy_icon_url", m.ProxyIconURL);
}

void from_json(const nlohmann::json &j, EmbedFieldData &m) {
    JS_D("name", m.Name);
    JS_D("value", m.Value);
    JS_O("inline", m.Inline);
}

void from_json(const nlohmann::json &j, EmbedData &m) {
    JS_O("title", m.Title);
    JS_O("type", m.Type);
    JS_O("description", m.Description);
    JS_O("url", m.URL);
    JS_O("timestamp", m.Timestamp);
    JS_O("color", m.Color);
    JS_O("footer", m.Footer);
    JS_O("image", m.Image);
    JS_O("thumbnail", m.Thumbnail);
    JS_O("video", m.Video);
    JS_O("provider", m.Provider);
    JS_O("author", m.Author);
    JS_O("fields", m.Fields);
}

void from_json(const nlohmann::json &j, MessageData &m) {
    JS_D("id", m.ID);
    JS_D("channel_id", m.ChannelID);
    JS_O("guild_id", m.GuildID);
    JS_D("author", m.Author);
    // JS_O("member", m.Member);
    JS_D("content", m.Content);
    JS_D("timestamp", m.Timestamp);
    JS_N("edited_timestamp", m.EditedTimestamp);
    JS_D("tts", m.IsTTS);
    JS_D("mention_everyone", m.DoesMentionEveryone);
    JS_D("mentions", m.Mentions);
    // JS_D("mention_roles", m.MentionRoles);
    // JS_O("mention_channels", m.MentionChannels);
    // JS_D("attachments", m.Attachments);
    JS_D("embeds", m.Embeds);
    // JS_O("reactions", m.Reactions);
    JS_O("nonce", m.Nonce);
    JS_D("pinned", m.IsPinned);
    JS_O("webhook_id", m.WebhookID);
    JS_D("type", m.Type);
    // JS_O("activity", m.Activity);
    // JS_O("application", m.Application);
    // JS_O("message_reference", m.MessageReference);
    JS_O("flags", m.Flags);
}

// probably gonna need to return present keys
void MessageData::from_json_edited(const nlohmann::json &j) {
    JS_D("id", ID);
    JS_D("channel_id", ChannelID);
    JS_O("guild_id", GuildID);
    JS_O("author", Author);
    JS_O("content", Content);
    JS_O("timestamp", Timestamp);
    JS_ON("edited_timestamp", EditedTimestamp);
    JS_O("tts", IsTTS);
    JS_O("mention_everyone", DoesMentionEveryone);
    JS_O("mentions", Mentions);
    JS_O("nonce", Nonce);
    JS_O("pinned", IsPinned);
    JS_O("webhook_id", WebhookID);
    JS_O("type", Type);
    JS_O("flags", Flags);
}
