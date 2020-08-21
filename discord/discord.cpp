#include "../abaddon.hpp"
#include "discord.hpp"
#include <cassert>

DiscordClient::DiscordClient()
    : m_http(DiscordAPI) {
    LoadEventMap();
}

void DiscordClient::SetAbaddon(Abaddon *ptr) {
    m_abaddon = ptr;
}

void DiscordClient::Start() {
    assert(!m_client_connected);
    assert(!m_websocket.IsOpen());

    m_client_connected = true;
    m_websocket.StartConnection(DiscordGateway);
    m_websocket.SetJSONCallback(std::bind(&DiscordClient::HandleGatewayMessage, this, std::placeholders::_1));
}

void DiscordClient::Stop() {
    std::scoped_lock<std::mutex> guard(m_mutex);
    if (!m_client_connected) return;

    m_heartbeat_waiter.kill();
    m_heartbeat_thread.join();
    m_client_connected = false;
    m_websocket.Stop();

    m_guilds.clear();
}

bool DiscordClient::IsStarted() const {
    return m_client_connected;
}

const DiscordClient::Guilds_t &DiscordClient::GetGuilds() const {
    std::scoped_lock<std::mutex> guard(m_mutex);
    return m_guilds;
}

const UserSettingsData &DiscordClient::GetUserSettings() const {
    std::scoped_lock<std::mutex> guard(m_mutex);
    assert(m_ready_received);
    return m_user_settings;
}

std::vector<std::pair<Snowflake, GuildData>> DiscordClient::GetUserSortedGuilds() const {
    std::vector<std::pair<Snowflake, GuildData>> sorted_guilds;

    if (m_user_settings.GuildPositions.size()) {
        for (const auto &id : m_user_settings.GuildPositions) {
            auto &guild = m_guilds.at(id);
            sorted_guilds.push_back(std::make_pair(id, guild));
        }
    } else { // default sort is alphabetic
        for (auto &it : m_guilds)
            sorted_guilds.push_back(it);
        std::sort(sorted_guilds.begin(), sorted_guilds.end(), [&](auto &a, auto &b) -> bool {
            std::string &s1 = a.second.Name;
            std::string &s2 = b.second.Name;

            if (s1.empty() || s2.empty())
                return s1 < s2;

            bool ac[] = {
                !isalnum(s1[0]),
                !isalnum(s2[0]),
                isdigit(s1[0]),
                isdigit(s2[0]),
                isalpha(s1[0]),
                isalpha(s2[0]),
            };

            if ((ac[0] && ac[1]) || (ac[2] && ac[3]) || (ac[4] && ac[5]))
                return s1 < s2;

            return ac[0] || ac[5];
        });
    }

    return sorted_guilds;
}

std::unordered_set<const MessageData *> DiscordClient::GetMessagesForChannel(Snowflake id) const {
    auto it = m_chan_to_message_map.find(id);
    if (it == m_chan_to_message_map.end())
        return std::unordered_set<const MessageData *>();
    return it->second;
}

void DiscordClient::UpdateSettingsGuildPositions(const std::vector<Snowflake> &pos) {
    assert(pos.size() == m_guilds.size());
    nlohmann::json body;
    body["guild_positions"] = pos;
    m_http.MakePATCH("/users/@me/settings", body.dump(), [this, pos](const cpr::Response &r) {
        m_user_settings.GuildPositions = pos;
        m_abaddon->DiscordNotifyChannelListFullRefresh();
    });
}

void DiscordClient::FetchMessagesInChannel(Snowflake id, std::function<void(const std::vector<MessageData> &)> cb) {
    std::string path = "/channels/" + std::to_string(id) + "/messages?limit=50";
    m_http.MakeGET(path, [this, id, cb](cpr::Response r) {
        std::vector<MessageData> msgs;
        nlohmann::json::parse(r.text).get_to(msgs);
        for (const auto &msg : msgs)
            StoreMessage(msg.ID, msg);

        cb(msgs);
    });
}

const MessageData *DiscordClient::GetMessage(Snowflake id) const {
    return &m_messages.at(id);
}

void DiscordClient::UpdateToken(std::string token) {
    m_token = token;
    m_http.SetAuth(token);
}

void DiscordClient::HandleGatewayMessage(nlohmann::json j) {
    GatewayMessage m;
    try {
        m = j;
    } catch (std::exception &e) {
        printf("Error decoding JSON. Discarding message: %s\n", e.what());
        return;
    }

    switch (m.Opcode) {
        case GatewayOp::Hello: {
            HelloMessageData d = m.Data;
            m_heartbeat_msec = d.HeartbeatInterval;
            assert(!m_heartbeat_thread.joinable()); // handle reconnects later
            m_heartbeat_thread = std::thread(std::bind(&DiscordClient::HeartbeatThread, this));
            SendIdentify();
        } break;
        case GatewayOp::HeartbeatAck: {
            m_heartbeat_acked = true;
        } break;
        case GatewayOp::Event: {
            auto iter = m_event_map.find(m.Type);
            if (iter == m_event_map.end()) {
                printf("Unknown event %s\n", m.Type.c_str());
                break;
            }
            switch (iter->second) {
                case GatewayEvent::READY: {
                    HandleGatewayReady(m);
                } break;
                case GatewayEvent::MESSAGE_CREATE: {
                    HandleGatewayMessageCreate(m);
                } break;
            }
        } break;
        default:
            printf("Unknown opcode %d\n", m.Opcode);
            break;
    }
}

void DiscordClient::HandleGatewayReady(const GatewayMessage &msg) {
    m_ready_received = true;
    ReadyEventData data = msg.Data;
    for (const auto &g : data.Guilds) {
        StoreGuild(g.ID, g);
    }
    m_abaddon->DiscordNotifyReady();
    m_user_settings = data.UserSettings;
}

void DiscordClient::HandleGatewayMessageCreate(const GatewayMessage& msg) {
    MessageData data = msg.Data;
    StoreMessage(data.ID, data);
    m_abaddon->DiscordNotifyMessageCreate(data.ID);
}

void DiscordClient::StoreGuild(Snowflake id, const GuildData &g) {
    assert(id.IsValid() && id == g.ID);
    m_guilds[id] = g;
}

void DiscordClient::StoreMessage(Snowflake id, const MessageData &m) {
    assert(id.IsValid());
    m_messages[id] = m;
    auto it = m_chan_to_message_map.find(m.ChannelID);
    if (it == m_chan_to_message_map.end())
        m_chan_to_message_map[m.ChannelID] = decltype(m_chan_to_message_map)::mapped_type();
    m_chan_to_message_map[m.ChannelID].insert(&m_messages[id]);
}

void DiscordClient::HeartbeatThread() {
    while (m_client_connected) {
        if (!m_heartbeat_acked) {
            printf("wow! a heartbeat wasn't acked! how could this happen?");
        }

        m_heartbeat_acked = false;

        HeartbeatMessage msg;
        msg.Sequence = m_last_sequence;
        nlohmann::json j = msg;
        m_websocket.Send(j);

        if (!m_heartbeat_waiter.wait_for(std::chrono::milliseconds(m_heartbeat_msec)))
            break;
    }
}

void DiscordClient::SendIdentify() {
    assert(m_token.size());
    IdentifyMessage msg;
    msg.Properties.OS = "OpenBSD";
    msg.Properties.Device = GatewayIdentity;
    msg.Properties.Browser = GatewayIdentity;
    msg.Token = m_token;
    m_websocket.Send(msg);
}

void DiscordClient::LoadEventMap() {
    m_event_map["READY"] = GatewayEvent::READY;
    m_event_map["MESSAGE_CREATE"] = GatewayEvent::MESSAGE_CREATE;
}

#define JS_D(k, t)         \
    do {                   \
        j.at(k).get_to(t); \
    } while (0)

#define JS_O(k, t)                            \
    do {                                      \
        if (j.contains(k)) j.at(k).get_to(t); \
    } while (0)

#define JS_N(k, t)                                 \
    do {                                           \
        if (!j.at(k).is_null()) j.at(k).get_to(t); \
    } while (0)

#define JS_ON(k, t)                              \
    do {                                         \
        if (j.contains(k) && !j.at(k).is_null()) \
            j.at(k).get_to(t);                   \
    } while (0)

void from_json(const nlohmann::json &j, GatewayMessage &m) {
    JS_D("op", m.Opcode);
    m.Data = j.at("d");

    JS_ON("t", m.Type);
}

void from_json(const nlohmann::json &j, HelloMessageData &m) {
    JS_D("heartbeat_interval", m.HeartbeatInterval);
}

void from_json(const nlohmann::json &j, UserData &m) {
    JS_D("id", m.ID);
    JS_D("username", m.Username);
    JS_D("discriminator", m.Discriminator);
    JS_N("avatar", m.Avatar);
    JS_O("bot", m.IsBot);
    JS_O("system", m.IsSystem);
    JS_O("mfa_enabled", m.IsMFAEnabled);
    JS_O("locale", m.Locale);
    JS_O("verified", m.IsVerified);
    JS_O("email", m.Email);
    JS_O("flags", m.Flags);
    JS_O("premium_type", m.PremiumType);
    JS_O("public_flags", m.PublicFlags);
    JS_O("desktop", m.IsDesktop);
    JS_O("mobile", m.IsMobile);
    JS_ON("nsfw_allowed", m.IsNSFWAllowed);
    JS_ON("phone", m.Phone);
}

void from_json(const nlohmann::json &j, GuildData &m) {
    JS_D("id", m.ID);
    JS_D("name", m.Name);
    JS_N("icon", m.Icon);
    JS_N("splash", m.Splash);
    JS_N("discovery_splash", m.DiscoverySplash);
    JS_O("owner", m.IsOwner);
    JS_D("owner_id", m.OwnerID);
    JS_O("permissions", m.Permissions);
    JS_O("permissions_new", m.PermissionsNew);
    JS_D("region", m.VoiceRegion);
    JS_N("afk_channel_id", m.AFKChannelID);
    JS_D("afk_timeout", m.AFKTimeout);
    JS_O("embed_enabled", m.IsEmbedEnabled);
    JS_ON("embed_channel_id", m.EmbedChannelID);
    JS_D("verification_level", m.VerificationLevel);
    JS_D("default_message_notifications", m.DefaultMessageNotifications);
    JS_D("explicit_content_filter", m.ExplicitContentFilter);
    // JS_D("roles", m.Roles);
    // JS_D("emojis", m.Emojis);
    JS_D("features", m.Features);
    JS_D("mfa_level", m.MFALevel);
    JS_N("application_id", m.ApplicationID);
    JS_O("widget_enabled", m.IsWidgetEnabled);
    JS_ON("widget_channel_id", m.WidgetChannelID);
    JS_N("system_channel_id", m.SystemChannelID);
    JS_D("system_channel_flags", m.SystemChannelFlags);
    JS_N("rules_channel_id", m.RulesChannelID);
    JS_O("joined_at", m.JoinedAt);
    JS_O("large", m.IsLarge);
    JS_O("unavailable", m.IsUnavailable);
    JS_O("member_count", m.MemberCount);
    // JS_O("voice_states", m.VoiceStates);
    // JS_O("members", m.Members);
    JS_O("channels", m.Channels);
    // JS_O("presences", m.Presences);
    JS_ON("max_presences", m.MaxPresences);
    JS_O("max_members", m.MaxMembers);
    JS_N("vanity_url_code", m.VanityURL);
    JS_N("description", m.Description);
    JS_N("banner", m.BannerHash);
    JS_D("premium_tier", m.PremiumTier);
    JS_O("premium_subscription_count", m.PremiumSubscriptionCount);
    JS_D("preferred_locale", m.PreferredLocale);
    JS_N("public_updates_channel_id", m.PublicUpdatesChannelID);
    JS_O("max_video_channel_users", m.MaxVideoChannelUsers);
    JS_O("approximate_member_count", m.ApproximateMemberCount);
    JS_O("approximate_presence_count", m.ApproximatePresenceCount);
}

void from_json(const nlohmann::json &j, ChannelData &m) {
    JS_D("id", m.ID);
    JS_D("type", m.Type);
    JS_O("guild_id", m.GuildID);
    JS_O("position", m.Position);
    // JS_O("permission_overwrites", m.PermissionOverwrites);
    JS_O("name", m.Name);
    JS_ON("topic", m.Topic);
    JS_O("nsfw", m.IsNSFW);
    JS_ON("last_message_id", m.LastMessageID);
    JS_O("bitrate", m.Bitrate);
    JS_O("user_limit", m.UserLimit);
    JS_O("rate_limit_per_user", m.RateLimitPerUser);
    JS_O("recipients", m.Recipients);
    JS_ON("icon", m.Icon);
    JS_O("owner_id", m.OwnerID);
    JS_O("application_id", m.ApplicationID);
    JS_ON("parent_id", m.ParentID);
    JS_ON("last_pin_timestamp", m.LastPinTimestamp);
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
    // JS_D("embeds", m.Embeds);
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

void from_json(const nlohmann::json &j, ReadyEventData &m) {
    JS_D("v", m.GatewayVersion);
    JS_D("user", m.User);
    JS_D("guilds", m.Guilds);
    JS_D("session_id", m.SessionID);
    JS_D("analytics_token", m.AnalyticsToken);
    JS_D("friend_suggestion_count", m.FriendSuggestionCount);
    JS_D("user_settings", m.UserSettings);
}

void from_json(const nlohmann::json &j, UserSettingsData &m) {
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

void to_json(nlohmann::json &j, const IdentifyProperties &m) {
    j["$os"] = m.OS;
    j["$browser"] = m.Browser;
    j["$device"] = m.Device;
}

void to_json(nlohmann::json &j, const IdentifyMessage &m) {
    j["op"] = GatewayOp::Identify;
    j["d"] = nlohmann::json::object();
    j["d"]["token"] = m.Token;
    j["d"]["properties"] = m.Properties;

    if (m.LargeThreshold)
        j["d"]["large_threshold"] = m.LargeThreshold;
}

void to_json(nlohmann::json &j, const HeartbeatMessage &m) {
    j["op"] = GatewayOp::Heartbeat;
    if (m.Sequence == -1)
        j["d"] = nullptr;
    else
        j["d"] = m.Sequence;
}

Snowflake::Snowflake()
    : m_num(Invalid) {}

Snowflake::Snowflake(const Snowflake &s)
    : m_num(s.m_num) {}

Snowflake::Snowflake(uint64_t n)
    : m_num(n) {}

Snowflake::Snowflake(const std::string &str) {
    if (str.size())
        m_num = std::stoull(str);
    else
        m_num = Invalid;
};

bool Snowflake::IsValid() const {
    return m_num != Invalid;
}

void from_json(const nlohmann::json &j, Snowflake &s) {
    std::string tmp;
    j.get_to(tmp);
    s.m_num = std::stoull(tmp);
}

void to_json(nlohmann::json &j, const Snowflake &s) {
    j = std::to_string(s);
}

#undef JS_O
#undef JS_D
#undef JS_N
#undef JS_ON
