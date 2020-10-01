#include "objects.hpp"

void from_json(const nlohmann::json &j, GatewayMessage &m) {
    JS_D("op", m.Opcode);
    m.Data = j.at("d");

    JS_ON("t", m.Type);
    JS_ON("s", m.Sequence);
}

void from_json(const nlohmann::json &j, HelloMessageData &m) {
    JS_D("heartbeat_interval", m.HeartbeatInterval);
}

void from_json(const nlohmann::json &j, MessageDeleteData &m) {
    JS_D("id", m.ID);
    JS_D("channel_id", m.ChannelID);
    JS_O("guild_id", m.GuildID);
}

void from_json(const nlohmann::json &j, MessageDeleteBulkData &m) {
    JS_D("ids", m.IDs);
    JS_D("channel_id", m.ChannelID);
    JS_O("guild_id", m.GuildID);
}

void from_json(const nlohmann::json &j, GuildMemberListUpdateMessage::GroupItem &m) {
    m.Type = "group";
    JS_D("id", m.ID);
    JS_D("count", m.Count);
}

GuildMember GuildMemberListUpdateMessage::MemberItem::GetAsMemberData() const {
    return m_member_data;
}

void from_json(const nlohmann::json &j, GuildMemberListUpdateMessage::MemberItem &m) {
    m.Type = "member";
    JS_D("user", m.User);
    JS_D("roles", m.Roles);
    JS_D("mute", m.IsMuted);
    JS_D("joined_at", m.JoinedAt);
    JS_D("deaf", m.IsDefeaned);
    JS_N("hoisted_role", m.HoistedRole);
    JS_ON("premium_since", m.PremiumSince);
    JS_ON("nick", m.Nickname);
    m.m_member_data = j;
}

void from_json(const nlohmann::json &j, GuildMemberListUpdateMessage::OpObject &m) {
    JS_D("op", m.Op);
    if (m.Op == "SYNC") {
        JS_D("range", m.Range);
        for (const auto &ij : j.at("items")) {
            if (ij.contains("group"))
                m.Items.push_back(std::make_unique<GuildMemberListUpdateMessage::GroupItem>(ij.at("group")));
            else if (ij.contains("member"))
                m.Items.push_back(std::make_unique<GuildMemberListUpdateMessage::MemberItem>(ij.at("member")));
        }
    }
}

void from_json(const nlohmann::json &j, GuildMemberListUpdateMessage &m) {
    JS_D("online_count", m.OnlineCount);
    JS_D("member_count", m.MemberCount);
    JS_D("id", m.ListIDHash);
    JS_D("guild_id", m.GuildID);
    JS_D("groups", m.Groups);
    JS_D("ops", m.Ops);
}

void to_json(nlohmann::json &j, const LazyLoadRequestMessage &m) {
    j["op"] = GatewayOp::LazyLoadRequest;
    j["d"] = nlohmann::json::object();
    j["d"]["guild_id"] = m.GuildID;
    j["d"]["channels"] = nlohmann::json::object();
    for (const auto &[key, chans] : m.Channels) { // apparently a map gets written as a list
        j["d"]["channels"][std::to_string(key)] = chans;
    }
    j["d"]["typing"] = m.ShouldGetTyping;
    j["d"]["activities"] = m.ShouldGetActivities;
    if (m.Members.size() > 0)
        j["d"]["members"] = m.Members;
}

void from_json(const nlohmann::json &j, ReadyEventData &m) {
    JS_D("v", m.GatewayVersion);
    JS_D("user", m.User);
    JS_D("guilds", m.Guilds);
    JS_D("session_id", m.SessionID);
    JS_D("analytics_token", m.AnalyticsToken);
    JS_D("friend_suggestion_count", m.FriendSuggestionCount);
    JS_D("user_settings", m.UserSettings);
    JS_D("private_channels", m.PrivateChannels);
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

void to_json(nlohmann::json &j, const CreateMessageObject &m) {
    j["content"] = m.Content;
}

void to_json(nlohmann::json &j, const MessageEditObject &m) {
    if (m.Content.size() > 0)
        j["content"] = m.Content;

    // todo EmbedData to_json
    // if (m.Embeds.size() > 0)
    //    j["embeds"] = m.Embeds;

    if (m.Flags != -1)
        j["flags"] = m.Flags;
}
