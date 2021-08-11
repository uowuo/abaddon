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
    JS_ON("presence", m.Presence);
    m.m_member_data = j;
}

void from_json(const nlohmann::json &j, GuildMemberListUpdateMessage::OpObject &m) {
    JS_D("op", m.Op);
    if (m.Op == "SYNC") {
        m.Items.emplace();
        JS_D("range", m.Range);
        for (const auto &ij : j.at("items")) {
            if (ij.contains("group"))
                m.Items->push_back(std::make_unique<GuildMemberListUpdateMessage::GroupItem>(ij.at("group")));
            else if (ij.contains("member"))
                m.Items->push_back(std::make_unique<GuildMemberListUpdateMessage::MemberItem>(ij.at("member")));
        }
    } else if (m.Op == "UPDATE") {
        JS_D("index", m.Index);
        const auto &ij = j.at("item");
        if (ij.contains("member"))
            m.OpItem = std::make_unique<GuildMemberListUpdateMessage::MemberItem>(ij.at("member"));
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
    if (m.Channels.has_value()) {
        j["d"]["channels"] = nlohmann::json::object();
        for (const auto &[key, chans] : *m.Channels)
            j["d"]["channels"][std::to_string(key)] = chans;
    }
    j["d"]["typing"] = m.ShouldGetTyping;
    j["d"]["activities"] = m.ShouldGetActivities;
    j["d"]["threads"] = m.ShouldGetThreads;
    if (m.Members.has_value())
        j["d"]["members"] = *m.Members;
}

void to_json(nlohmann::json &j, const UpdateStatusMessage &m) {
    j["op"] = GatewayOp::UpdateStatus;
    j["d"] = nlohmann::json::object();
    j["d"]["since"] = m.Since;
    j["d"]["activities"] = m.Activities;
    j["d"]["afk"] = m.IsAFK;
    switch (m.Status) {
        case PresenceStatus::Online:
            j["d"]["status"] = "online";
            break;
        case PresenceStatus::Offline:
            j["d"]["status"] = "invisible";
            break;
        case PresenceStatus::Idle:
            j["d"]["status"] = "idle";
            break;
        case PresenceStatus::DND:
            j["d"]["status"] = "dnd";
            break;
    }
}

void from_json(const nlohmann::json &j, ReadyEventData &m) {
    JS_D("v", m.GatewayVersion);
    JS_D("user", m.SelfUser);
    JS_D("guilds", m.Guilds);
    JS_D("session_id", m.SessionID);
    JS_O("analytics_token", m.AnalyticsToken);
    JS_O("friend_suggestion_count", m.FriendSuggestionCount);
    JS_D("user_settings", m.Settings);
    JS_D("private_channels", m.PrivateChannels);
    JS_O("users", m.Users);
    JS_ON("merged_members", m.MergedMembers);
    JS_O("relationships", m.Relationships);
    JS_O("guild_join_requests", m.GuildJoinRequests);
}

void from_json(const nlohmann::json &j, MergedPresence &m) {
    JS_D("user_id", m.UserID);
    JS_O("last_modified", m.LastModified);
    m.Presence = j;
}

void from_json(const nlohmann::json &j, SupplementalMergedPresencesData &m) {
    JS_D("guilds", m.Guilds);
    JS_D("friends", m.Friends);
}

void from_json(const nlohmann::json &j, ReadySupplementalData &m) {
    JS_D("merged_presences", m.MergedPresences);
}

void to_json(nlohmann::json &j, const IdentifyProperties &m) {
    j["os"] = m.OS;
    j["browser"] = m.Browser;
    j["device"] = m.Device;
    j["system_locale"] = m.SystemLocale;
    j["browser_user_agent"] = m.BrowserUserAgent;
    j["browser_version"] = m.BrowserVersion;
    j["os_version"] = m.OSVersion;
    j["referrer"] = m.Referrer;
    j["referring_domain"] = m.ReferringDomain;
    j["referrer_current"] = m.ReferrerCurrent;
    j["referring_domain_current"] = m.ReferringDomainCurrent;
    j["release_channel"] = m.ReleaseChannel;
    j["client_build_number"] = m.ClientBuildNumber;
    if (m.ClientEventSource == "")
        j["client_event_source"] = nullptr;
    else
        j["client_event_source"] = m.ClientEventSource;
}

void to_json(nlohmann::json &j, const ClientStateProperties &m) {
    j["guild_hashes"] = m.GuildHashes;
    j["highest_last_message_id"] = m.HighestLastMessageID;
    j["read_state_version"] = m.ReadStateVersion;
    j["user_guild_settings_version"] = m.UserGuildSettingsVersion;
}

void to_json(nlohmann::json &j, const IdentifyMessage &m) {
    j["op"] = GatewayOp::Identify;
    j["d"] = nlohmann::json::object();
    j["d"]["token"] = m.Token;
    j["d"]["capabilities"] = m.Capabilities;
    j["d"]["properties"] = m.Properties;
    j["d"]["presence"] = m.Presence;
    j["d"]["compress"] = m.DoesSupportCompression;
    j["d"]["client_state"] = m.ClientState;
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
    JS_IF("message_reference", m.MessageReference);
    JS_IF("nonce", m.Nonce);
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

void from_json(const nlohmann::json &j, GuildMemberUpdateMessage &m) {
    JS_D("guild_id", m.GuildID);
    JS_D("roles", m.Roles);
    JS_D("user", m.User);
    JS_ON("nick", m.Nick);
    JS_D("joined_at", m.JoinedAt);
}

void from_json(const nlohmann::json &j, ClientStatusData &m) {
    JS_O("desktop", m.Desktop);
    JS_O("mobile", m.Mobile);
    JS_O("web", m.Web);
}

void from_json(const nlohmann::json &j, PresenceUpdateMessage &m) {
    m.User = j.at("user");
    JS_O("guild_id", m.GuildID);
    JS_D("status", m.StatusMessage);
    JS_D("activities", m.Activities);
    JS_D("client_status", m.ClientStatus);
}

void to_json(nlohmann::json &j, const CreateDMObject &m) {
    std::vector<std::string> conv;
    for (const auto &id : m.Recipients)
        conv.push_back(std::to_string(id));
    j["recipients"] = conv;
}

void to_json(nlohmann::json &j, const ResumeMessage &m) {
    j["op"] = GatewayOp::Resume;
    j["d"] = nlohmann::json::object();
    j["d"]["token"] = m.Token;
    j["d"]["session_id"] = m.SessionID;
    j["d"]["seq"] = m.Sequence;
}

void from_json(const nlohmann::json &j, GuildRoleUpdateObject &m) {
    JS_D("guild_id", m.GuildID);
    JS_D("role", m.Role);
}

void from_json(const nlohmann::json &j, GuildRoleCreateObject &m) {
    JS_D("guild_id", m.GuildID);
    JS_D("role", m.Role);
}

void from_json(const nlohmann::json &j, GuildRoleDeleteObject &m) {
    JS_D("guild_id", m.GuildID);
    JS_D("role_id", m.RoleID);
}

void from_json(const nlohmann::json &j, MessageReactionAddObject &m) {
    JS_D("user_id", m.UserID);
    JS_D("channel_id", m.ChannelID);
    JS_D("message_id", m.MessageID);
    JS_O("guild_id", m.GuildID);
    JS_O("member", m.Member);
    JS_D("emoji", m.Emoji);
}

void from_json(const nlohmann::json &j, MessageReactionRemoveObject &m) {
    JS_D("user_id", m.UserID);
    JS_D("channel_id", m.ChannelID);
    JS_D("message_id", m.MessageID);
    JS_O("guild_id", m.GuildID);
    JS_D("emoji", m.Emoji);
}

void from_json(const nlohmann::json &j, ChannelRecipientAdd &m) {
    JS_D("user", m.User);
    JS_D("channel_id", m.ChannelID);
}

void from_json(const nlohmann::json &j, ChannelRecipientRemove &m) {
    JS_D("user", m.User);
    JS_D("channel_id", m.ChannelID);
}

void from_json(const nlohmann::json &j, TypingStartObject &m) {
    JS_D("channel_id", m.ChannelID);
    JS_O("guild_id", m.GuildID);
    JS_D("user_id", m.UserID);
    JS_D("timestamp", m.Timestamp);
    JS_O("member", m.Member);
}

void to_json(nlohmann::json &j, const ModifyGuildObject &m) {
    JS_IF("name", m.Name);
    JS_IF("icon", m.IconData);
}

void from_json(const nlohmann::json &j, GuildBanRemoveObject &m) {
    JS_D("guild_id", m.GuildID);
    JS_D("user", m.User);
}

void from_json(const nlohmann::json &j, GuildBanAddObject &m) {
    JS_D("guild_id", m.GuildID);
    JS_D("user", m.User);
}

void from_json(const nlohmann::json &j, InviteCreateObject &m) {
    JS_D("channel_id", m.ChannelID);
    JS_D("code", m.Code);
    JS_D("created_at", m.CreatedAt);
    JS_O("guild_id", m.GuildID);
    JS_O("inviter", m.Inviter);
    JS_D("max_age", m.MaxAge);
    JS_D("max_uses", m.MaxUses);
    JS_O("target_user", m.TargetUser);
    JS_O("target_user_type", m.TargetUserType);
    JS_D("temporary", m.IsTemporary);
    JS_D("uses", m.Uses);
}

void from_json(const nlohmann::json &j, InviteDeleteObject &m) {
    JS_D("channel_id", m.ChannelID);
    JS_O("guild_id", m.GuildID);
    JS_D("code", m.Code);
}

void from_json(const nlohmann::json &j, ConnectionData &m) {
    JS_D("id", m.ID);
    JS_D("type", m.Type);
    JS_D("name", m.Name);
    JS_D("verified", m.IsVerified);
}

void from_json(const nlohmann::json &j, MutualGuildData &m) {
    JS_D("id", m.ID);
    JS_ON("nick", m.Nick);
}

void from_json(const nlohmann::json &j, UserProfileData &m) {
    JS_D("connected_accounts", m.ConnectedAccounts);
    JS_D("mutual_guilds", m.MutualGuilds);
    JS_ON("premium_guild_since", m.PremiumGuildSince);
    JS_ON("premium_since", m.PremiumSince);
    JS_D("user", m.User);
}

void from_json(const nlohmann::json &j, UserNoteObject &m) {
    JS_ON("note", m.Note);
    JS_ON("note_user_id", m.NoteUserID);
    JS_ON("user_id", m.UserID);
}

void to_json(nlohmann::json &j, const UserSetNoteObject &m) {
    j["note"] = m.Note;
}

void from_json(const nlohmann::json &j, UserNoteUpdateMessage &m) {
    JS_D("note", m.Note);
    JS_D("id", m.ID);
}

void from_json(const nlohmann::json &j, RelationshipsData &m) {
    j.get_to(m.Users);
}

void to_json(nlohmann::json &j, const ModifyGuildMemberObject &m) {
    JS_IF("roles", m.Roles);
}

void to_json(nlohmann::json &j, const ModifyGuildRoleObject &m) {
    JS_IF("name", m.Name);
    JS_IF("color", m.Color);
    JS_IF("hoist", m.IsHoisted);
    JS_IF("mentionable", m.Mentionable);
    if (m.Permissions.has_value())
        j["permissions"] = std::to_string(static_cast<uint64_t>(*m.Permissions));
}

void to_json(nlohmann::json &j, const ModifyGuildRolePositionsObject::PositionParam &m) {
    j["id"] = m.ID;
    JS_IF("position", m.Position);
}

void to_json(nlohmann::json &j, const ModifyGuildRolePositionsObject &m) {
    j = m.Positions;
}

void from_json(const nlohmann::json &j, GuildEmojisUpdateObject &m) {
    JS_D("guild_id", m.GuildID);
}

void to_json(nlohmann::json &j, const ModifyGuildEmojiObject &m) {
    JS_IF("name", m.Name);
}

void from_json(const nlohmann::json &j, GuildJoinRequestCreateData &m) {
    auto tmp = j.at("status").get<std::string_view>();
    if (tmp == "STARTED")
        m.Status = GuildApplicationStatus::STARTED;
    else if (tmp == "PENDING")
        m.Status = GuildApplicationStatus::PENDING;
    else if (tmp == "REJECTED")
        m.Status = GuildApplicationStatus::REJECTED;
    else if (tmp == "APPROVED")
        m.Status = GuildApplicationStatus::APPROVED;
    JS_D("request", m.Request);
    JS_D("guild_id", m.GuildID);
}

void from_json(const nlohmann::json &j, GuildJoinRequestDeleteData &m) {
    JS_D("user_id", m.UserID);
    JS_D("guild_id", m.GuildID);
}

void from_json(const nlohmann::json &j, VerificationFieldObject &m) {
    JS_D("field_type", m.Type);
    JS_D("label", m.Label);
    JS_D("required", m.Required);
    JS_D("values", m.Values);
}

void from_json(const nlohmann::json &j, VerificationGateInfoObject &m) {
    JS_O("description", m.Description);
    JS_O("form_fields", m.VerificationFields);
    JS_O("version", m.Version);
    JS_O("enabled", m.Enabled);
}

void to_json(nlohmann::json &j, const VerificationFieldObject &m) {
    j["field_type"] = m.Type;
    j["label"] = m.Label;
    j["required"] = m.Required;
    j["values"] = m.Values;
    JS_IF("response", m.Response);
}

void to_json(nlohmann::json &j, const VerificationGateInfoObject &m) {
    JS_IF("description", m.Description);
    JS_IF("form_fields", m.VerificationFields);
    JS_IF("version", m.Version);
    JS_IF("enabled", m.Enabled);
}

void from_json(const nlohmann::json &j, RateLimitedResponse &m) {
    JS_D("code", m.Code);
    JS_D("global", m.Global);
    JS_O("message", m.Message);
    JS_D("retry_after", m.RetryAfter);
}

void from_json(const nlohmann::json &j, RelationshipRemoveData &m) {
    JS_D("id", m.ID);
    JS_D("type", m.Type);
}

void from_json(const nlohmann::json &j, RelationshipAddData &m) {
    JS_D("id", m.ID);
    JS_D("type", m.Type);
    JS_D("user", m.User);
}

void to_json(nlohmann::json &j, const FriendRequestObject &m) {
    j["username"] = m.Username;
    j["discriminator"] = m.Discriminator;
}

void to_json(nlohmann::json &j, const PutRelationshipObject &m) {
    JS_IF("type", m.Type);
}

void from_json(const nlohmann::json &j, ThreadCreateData &m) {
    j.get_to(m.Channel);
}

void from_json(const nlohmann::json &j, ThreadDeleteData &m) {
    JS_D("id", m.ID);
    JS_D("guild_id", m.GuildID);
    JS_D("parent_id", m.ParentID);
    JS_D("type", m.Type);
}

void from_json(const nlohmann::json &j, ThreadListSyncData &m) {
    JS_D("threads", m.Threads);
    JS_D("guild_id", m.GuildID);
}

void from_json(const nlohmann::json &j, ThreadMembersUpdateData &m) {
    JS_D("id", m.ID);
    JS_D("guild_id", m.GuildID);
    JS_D("member_count", m.MemberCount);
    JS_O("added_members", m.AddedMembers);
    JS_O("removed_member_ids", m.RemovedMemberIDs);
}

void from_json(const nlohmann::json &j, ArchivedThreadsResponseData &m) {
    JS_D("threads", m.Threads);
    JS_D("members", m.Members);
    JS_D("has_more", m.HasMore);
}
