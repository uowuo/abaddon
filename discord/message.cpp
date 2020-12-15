#include "message.hpp"

void to_json(nlohmann::json &j, const EmbedFooterData &m) {
    j["text"] = m.Text;
    JS_IF("icon_url", m.IconURL);
    JS_IF("proxy_icon_url", m.ProxyIconURL);
}

void from_json(const nlohmann::json &j, EmbedFooterData &m) {
    JS_D("text", m.Text);
    JS_O("icon_url", m.IconURL);
    JS_O("proxy_icon_url", m.ProxyIconURL);
}

void to_json(nlohmann::json &j, const EmbedImageData &m) {
    JS_IF("url", m.URL);
    JS_IF("proxy_url", m.ProxyURL);
    JS_IF("height", m.Height);
    JS_IF("width", m.Width);
}

void from_json(const nlohmann::json &j, EmbedImageData &m) {
    JS_O("url", m.URL);
    JS_O("proxy_url", m.ProxyURL);
    JS_O("height", m.Height);
    JS_O("width", m.Width);
}

void to_json(nlohmann::json &j, const EmbedThumbnailData &m) {
    JS_IF("url", m.URL);
    JS_IF("proxy_url", m.ProxyURL);
    JS_IF("height", m.Height);
    JS_IF("width", m.Width);
}

void from_json(const nlohmann::json &j, EmbedThumbnailData &m) {
    JS_O("url", m.URL);
    JS_O("proxy_url", m.ProxyURL);
    JS_O("height", m.Height);
    JS_O("width", m.Width);
}

void to_json(nlohmann::json &j, const EmbedVideoData &m) {
    JS_IF("url", m.URL);
    JS_IF("height", m.Height);
    JS_IF("width", m.Width);
}

void from_json(const nlohmann::json &j, EmbedVideoData &m) {
    JS_O("url", m.URL);
    JS_O("height", m.Height);
    JS_O("width", m.Width);
}

void to_json(nlohmann::json &j, const EmbedProviderData &m) {
    JS_IF("name", m.Name);
    JS_IF("url", m.URL);
}

void from_json(const nlohmann::json &j, EmbedProviderData &m) {
    JS_O("name", m.Name);
    JS_ON("url", m.URL);
}

void to_json(nlohmann::json &j, const EmbedAuthorData &m) {
    JS_IF("name", m.Name);
    JS_IF("url", m.URL);
    JS_IF("icon_url", m.IconURL);
    JS_IF("proxy_icon_url", m.ProxyIconURL);
}

void from_json(const nlohmann::json &j, EmbedAuthorData &m) {
    JS_O("name", m.Name);
    JS_O("url", m.URL);
    JS_O("icon_url", m.IconURL);
    JS_O("proxy_icon_url", m.ProxyIconURL);
}

void to_json(nlohmann::json &j, const EmbedFieldData &m) {
    j["name"] = m.Name;
    j["value"] = m.Value;
    JS_IF("inline", m.Inline);
}

void from_json(const nlohmann::json &j, EmbedFieldData &m) {
    JS_D("name", m.Name);
    JS_D("value", m.Value);
    JS_O("inline", m.Inline);
}

void to_json(nlohmann::json &j, const EmbedData &m) {
    JS_IF("title", m.Title);
    JS_IF("type", m.Type);
    JS_IF("description", m.Description);
    JS_IF("url", m.URL);
    JS_IF("timestamp", m.Timestamp);
    JS_IF("color", m.Color);
    JS_IF("footer", m.Footer);
    JS_IF("image", m.Image);
    JS_IF("thumbnail", m.Thumbnail);
    JS_IF("video", m.Video);
    JS_IF("provider", m.Provider);
    JS_IF("author", m.Author);
    JS_IF("fields", m.Fields);
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

void to_json(nlohmann::json &j, const AttachmentData &m) {
    j["id"] = m.ID;
    j["filename"] = m.Filename;
    j["size"] = m.Bytes;
    j["url"] = m.URL;
    j["proxy_url"] = m.ProxyURL;
    JS_IF("height", m.Height);
    JS_IF("width", m.Width);
}

void from_json(const nlohmann::json &j, AttachmentData &m) {
    JS_D("id", m.ID);
    JS_D("filename", m.Filename);
    JS_D("size", m.Bytes);
    JS_D("url", m.URL);
    JS_D("proxy_url", m.ProxyURL);
    JS_ON("height", m.Height);
    JS_ON("width", m.Width);
}

void from_json(const nlohmann::json &j, MessageReferenceData &m) {
    JS_O("message_id", m.MessageID);
    JS_O("channel_id", m.ChannelID);
    JS_O("guild_id", m.GuildID);
}

void to_json(nlohmann::json &j, const MessageReferenceData &m) {
    JS_IF("message_id", m.MessageID);
    JS_IF("channel_id", m.ChannelID);
    JS_IF("guild_id", m.GuildID);
}

void from_json(const nlohmann::json &j, ReactionData &m) {
    JS_D("count", m.Count);
    JS_D("me", m.HasReactedWith);
    JS_D("emoji", m.Emoji);
}

void to_json(nlohmann::json &j, const ReactionData &m) {
    j["count"] = m.Count;
    j["me"] = m.HasReactedWith;
    j["emoji"] = m.Emoji;
}

void from_json(const nlohmann::json &j, Message &m) {
    JS_D("id", m.ID);
    JS_D("channel_id", m.ChannelID);
    JS_O("guild_id", m.GuildID);
    JS_D("author", m.Author);
    // JS_O("member", m.Member);
    JS_D("content", m.Content);
    JS_D("timestamp", m.Timestamp);
    JS_N("edited_timestamp", m.EditedTimestamp);
    if (!j.at("edited_timestamp").is_null())
        m.SetEdited();
    JS_D("tts", m.IsTTS);
    JS_D("mention_everyone", m.DoesMentionEveryone);
    JS_D("mentions", m.Mentions);
    // JS_D("mention_roles", m.MentionRoles);
    // JS_O("mention_channels", m.MentionChannels);
    JS_D("attachments", m.Attachments);
    JS_D("embeds", m.Embeds);
    JS_O("reactions", m.Reactions);
    JS_O("nonce", m.Nonce);
    JS_D("pinned", m.IsPinned);
    JS_O("webhook_id", m.WebhookID);
    JS_D("type", m.Type);
    // JS_O("activity", m.Activity);
    // JS_O("application", m.Application);
    JS_O("message_reference", m.MessageReference);
    JS_O("flags", m.Flags);
    JS_O("stickers", m.Stickers);
}

void Message::from_json_edited(const nlohmann::json &j) {
    JS_D("id", ID);
    JS_D("channel_id", ChannelID);
    JS_O("guild_id", GuildID);
    JS_O("author", Author);
    JS_O("content", Content);
    JS_O("timestamp", Timestamp);
    JS_ON("edited_timestamp", EditedTimestamp);
    if (EditedTimestamp.size() > 0)
        SetEdited();
    JS_O("tts", IsTTS);
    JS_O("mention_everyone", DoesMentionEveryone);
    JS_O("mentions", Mentions);
    JS_O("embeds", Embeds);
    JS_O("nonce", Nonce);
    JS_O("pinned", IsPinned);
    JS_O("webhook_id", WebhookID);
    JS_O("type", Type);
    JS_O("flags", Flags);
    JS_O("stickers", Stickers);
}

void Message::SetDeleted() {
    m_deleted = true;
}

void Message::SetEdited() {
    m_edited = true;
}

bool Message::IsDeleted() const {
    return m_deleted;
}

bool Message::IsEdited() const {
    return m_edited;
}
