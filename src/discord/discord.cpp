#include "discord.hpp"

#include <cinttypes>
#include <utility>

#include <spdlog/spdlog.h>

#include "abaddon.hpp"

using namespace std::string_literals;

DiscordClient::DiscordClient(bool mem_store)
    : m_decompress_buf(InflateChunkSize)
    , m_store(mem_store)
    , m_websocket("gateway-ws") {
    m_msg_dispatch.connect(sigc::mem_fun(*this, &DiscordClient::MessageDispatch));
    auto dispatch_cb = [this]() {
        m_generic_mutex.lock();
        auto func = m_generic_queue.front();
        m_generic_queue.pop();
        m_generic_mutex.unlock();
        func();
    };
    m_generic_dispatch.connect(dispatch_cb);

    m_websocket.signal_message().connect(sigc::mem_fun(*this, &DiscordClient::HandleGatewayMessageRaw));
    m_websocket.signal_open().connect(sigc::mem_fun(*this, &DiscordClient::HandleSocketOpen));
    m_websocket.signal_close().connect(sigc::mem_fun(*this, &DiscordClient::HandleSocketClose));

#ifdef WITH_VOICE
    // hack: tracking signal objects instead of ensuring m_voice doesnt emit during destruction
    m_voice.signal_connected().connect(sigc::mem_fun(*this, &DiscordClient::OnVoiceConnected));
    m_voice.signal_disconnected().connect(sigc::track_obj(sigc::mem_fun(*this, &DiscordClient::OnVoiceDisconnected), m_signal_voice_disconnected));
    m_voice.signal_speaking().connect([this](const VoiceSpeakingData &data) {
        m_signal_voice_speaking.emit(data);
    });
    const auto signal_state_update_cb = [this](DiscordVoiceClient::State state) {
        m_signal_voice_client_state_update.emit(state);
    };
    m_voice.signal_state_update().connect(sigc::track_obj(signal_state_update_cb, m_signal_voice_client_state_update));
#endif

    LoadEventMap();
}

void DiscordClient::Start() {
    if (m_client_started) return;

    m_http.SetBase(GetAPIURL());
    SetHeaders();

    std::memset(&m_zstream, 0, sizeof(m_zstream));
    inflateInit2(&m_zstream, MAX_WBITS + 32);

    m_last_sequence = -1;
    m_heartbeat_acked = true;
    m_client_connected = true;
    m_client_started = true;
    m_websocket.StartConnection(GetGatewayURL());
}

bool DiscordClient::Stop() {
    if (m_client_started) {
        inflateEnd(&m_zstream);
        m_compressed_buf.clear();

        m_heartbeat_waiter.kill();
        if (m_heartbeat_thread.joinable()) m_heartbeat_thread.join();
        m_client_connected = false;
        m_reconnecting = false;

        m_store.ClearAll();
        m_guild_to_users.clear();

        m_websocket.Stop();

        m_client_started = false;

        return true;
    }

    m_client_started = false;

    return false;
}

bool DiscordClient::IsStarted() const {
    return m_client_started;
}

bool DiscordClient::IsStoreValid() const {
    return m_store.IsValid();
}

std::unordered_set<Snowflake> DiscordClient::GetGuilds() const {
    return m_store.GetGuilds();
}

const UserData &DiscordClient::GetUserData() const {
    return m_user_data;
}

const UserGuildSettingsData &DiscordClient::GetUserGuildSettings() const {
    return m_user_guild_settings;
}

std::optional<UserGuildSettingsEntry> DiscordClient::GetSettingsForGuild(Snowflake id) const {
    if (const auto it = m_user_guild_settings.Entries.find(id); it != m_user_guild_settings.Entries.end()) {
        return it->second;
    }

    return std::nullopt;
}

std::vector<Snowflake> DiscordClient::GetUserSortedGuilds() const {
    // sort order is unfolder'd guilds sorted by id descending, then guilds in folders in array order
    // todo: make sure folder'd guilds are sorted properly
    std::vector<Snowflake> folder_order;
    auto guilds = GetGuilds();
    for (const auto &entry : m_user_settings.GuildFolders) { // can contain guilds not a part of
        for (const auto &id : entry.GuildIDs) {
            if (guilds.find(id) != guilds.end())
                folder_order.push_back(id);
        }
    }

    std::vector<Snowflake> ret;
    for (const auto &gid : guilds) {
        if (std::find(folder_order.begin(), folder_order.end(), gid) == folder_order.end()) {
            ret.push_back(gid);
        }
    }

    std::sort(ret.rbegin(), ret.rend());

    for (const auto &gid : folder_order)
        ret.push_back(gid);

    return ret;
}

std::vector<Message> DiscordClient::GetMessagesForChannel(Snowflake id, size_t limit) const {
    return m_store.GetLastMessages(id, limit);
}

std::vector<Message> DiscordClient::GetMessagesBefore(Snowflake channel_id, Snowflake message_id, size_t limit) const {
    return m_store.GetMessagesBefore(channel_id, message_id, limit);
}

void DiscordClient::FetchInvite(const std::string &code, const sigc::slot<void(std::optional<InviteData>)> &callback) {
    m_http.MakeGET("/invites/" + code + "?with_counts=true", [callback](http::response_type r) {
        if (!CheckCode(r)) {
            if (r.status_code == 404)
                callback(std::nullopt);
            return;
        }

        callback(nlohmann::json::parse(r.text).get<InviteData>());
    });
}

void DiscordClient::FetchMessagesInChannel(Snowflake id, const sigc::slot<void(const std::vector<Message> &)> &cb) {
    std::string path = "/channels/" + std::to_string(id) + "/messages?limit=50";
    m_http.MakeGET(path, [this, id, cb](const http::response_type &r) {
        if (!CheckCode(r)) {
            // fake a thread delete event if the requested channel is a thread and we get a 404

            if (r.status_code == http::NotFound) {
                const auto channel = m_store.GetChannel(id);
                if (channel.has_value() && channel->IsThread()) {
                    ThreadDeleteData data;
                    data.GuildID = *channel->GuildID;
                    data.ID = id;
                    data.ParentID = *channel->ParentID;
                    data.Type = channel->Type;
                    m_signal_thread_delete.emit(data);
                }
            }

            return;
        }

        std::vector<Message> msgs;

        nlohmann::json::parse(r.text).get_to(msgs);

        m_store.BeginTransaction();
        for (auto &msg : msgs) {
            StoreMessageData(msg);
            if (msg.GuildID.has_value())
                AddUserToGuild(msg.Author.ID, *msg.GuildID);
        }
        m_store.EndTransaction();

        cb(msgs);
    });
}

void DiscordClient::FetchMessagesInChannelBefore(Snowflake channel_id, Snowflake before_id, const sigc::slot<void(const std::vector<Message> &)> &cb) {
    std::string path = "/channels/" + std::to_string(channel_id) + "/messages?limit=50&before=" + std::to_string(before_id);
    m_http.MakeGET(path, [this, cb](http::response_type r) {
        if (!CheckCode(r)) return;

        std::vector<Message> msgs;

        nlohmann::json::parse(r.text).get_to(msgs);

        m_store.BeginTransaction();
        for (auto &msg : msgs) {
            StoreMessageData(msg);
            if (msg.GuildID.has_value())
                AddUserToGuild(msg.Author.ID, *msg.GuildID);
        }
        m_store.EndTransaction();

        std::sort(msgs.begin(), msgs.end(), [](const Message &a, const Message &b) { return a.ID < b.ID; });
        cb(msgs);
    });
}

std::optional<Message> DiscordClient::GetMessage(Snowflake id) const {
    return m_store.GetMessage(id);
}

std::optional<ChannelData> DiscordClient::GetChannel(Snowflake id) const {
    return m_store.GetChannel(id);
}

std::optional<UserData> DiscordClient::GetUser(Snowflake id) const {
    return m_store.GetUser(id);
}

std::optional<RoleData> DiscordClient::GetRole(Snowflake id) const {
    return m_store.GetRole(id);
}

std::optional<GuildData> DiscordClient::GetGuild(Snowflake id) const {
    return m_store.GetGuild(id);
}

std::optional<GuildMember> DiscordClient::GetMember(Snowflake user_id, Snowflake guild_id) const {
    return m_store.GetGuildMember(guild_id, user_id);
}

std::optional<PermissionOverwrite> DiscordClient::GetPermissionOverwrite(Snowflake channel_id, Snowflake id) const {
    return m_store.GetPermissionOverwrite(channel_id, id);
}

std::optional<EmojiData> DiscordClient::GetEmoji(Snowflake id) const {
    return m_store.GetEmoji(id);
}

Snowflake DiscordClient::GetMemberHoistedRole(Snowflake guild_id, Snowflake user_id, bool with_color) const {
    const auto data = GetMember(user_id, guild_id);
    if (!data.has_value()) return Snowflake::Invalid;

    std::optional<RoleData> top_role;
    for (const auto &id : data->Roles) {
        const auto role = GetRole(id);
        if (role.has_value()) {
            if ((with_color && role->Color != 0x000000) || (!with_color && role->IsHoisted))
                if (!top_role.has_value() || top_role->Position < role->Position)
                    top_role = role;
        }
    }

    return top_role.has_value() ? top_role->ID : Snowflake::Invalid;
}

std::optional<RoleData> DiscordClient::GetMemberHoistedRoleCached(const GuildMember &member, const std::unordered_map<Snowflake, RoleData> &roles, bool with_color) const {
    std::optional<RoleData> top_role;
    for (const auto id : member.Roles) {
        if (const auto iter = roles.find(id); iter != roles.end()) {
            const auto &role = iter->second;
            if ((with_color && role.Color != 0x000000) || (!with_color && role.IsHoisted)) {
                if (!top_role.has_value() || top_role->Position < role.Position) {
                    top_role = role;
                }
            }
        }
    }
    return top_role;
}

std::optional<RoleData> DiscordClient::GetMemberHighestRole(Snowflake guild_id, Snowflake user_id) const {
    const auto data = GetMember(user_id, guild_id);
    if (!data.has_value()) return std::nullopt;

    if (data->Roles.empty()) return std::nullopt;
    if (data->Roles.size() == 1) return GetRole(data->Roles[0]);

    std::vector<RoleData> roles;
    for (const auto id : data->Roles)
        roles.push_back(*GetRole(id));

    return *std::max_element(roles.begin(), roles.end(), [](const auto &a, const auto &b) -> bool {
        return a.Position < b.Position;
    });
}

std::set<Snowflake> DiscordClient::GetUsersInGuild(Snowflake id) const {
    auto it = m_guild_to_users.find(id);
    if (it != m_guild_to_users.end())
        return it->second;

    return {};
}

std::set<Snowflake> DiscordClient::GetChannelsInGuild(Snowflake id) const {
    auto it = m_guild_to_channels.find(id);
    if (it != m_guild_to_channels.end())
        return it->second;
    return {};
}

std::vector<Snowflake> DiscordClient::GetUsersInThread(Snowflake id) const {
    if (auto it = m_thread_members.find(id); it != m_thread_members.end())
        return it->second;
    return {};
}

// there is an endpoint for this but it should be synced before this is called anyways
std::vector<ChannelData> DiscordClient::GetActiveThreads(Snowflake channel_id) const {
    return m_store.GetActiveThreads(channel_id);
}

void DiscordClient::GetArchivedPublicThreads(Snowflake channel_id, const sigc::slot<void(DiscordError, const ArchivedThreadsResponseData &)> &callback) {
    m_http.MakeGET("/channels/" + std::to_string(channel_id) + "/threads/archived/public", [this, callback](const http::response_type &r) {
        if (CheckCode(r)) {
            const auto data = nlohmann::json::parse(r.text).get<ArchivedThreadsResponseData>();
            for (const auto &thread : data.Threads)
                m_store.SetChannel(thread.ID, thread);
            callback(DiscordError::NONE, data);
        } else {
            callback(GetCodeFromResponse(r), {});
        }
    });
}

void DiscordClient::GetArchivedPrivateThreads(Snowflake channel_id, const sigc::slot<void(DiscordError, const ArchivedThreadsResponseData &)> &callback) {
    m_http.MakeGET("/channels/" + std::to_string(channel_id) + "/users/@me/threads/archived/private", [this, callback](const http::response_type &r) {
        if (CheckCode(r)) {
            const auto data = nlohmann::json::parse(r.text).get<ArchivedThreadsResponseData>();
            for (const auto &thread : data.Threads)
                m_store.SetChannel(thread.ID, thread);
            callback(DiscordError::NONE, data);
        } else {
            callback(GetCodeFromResponse(r), {});
        }
    });
}

std::vector<Snowflake> DiscordClient::GetChildChannelIDs(Snowflake parent_id) const {
    return m_store.GetChannelIDsWithParentID(parent_id);
}

std::optional<WebhookMessageData> DiscordClient::GetWebhookMessageData(Snowflake message_id) const {
    return m_store.GetWebhookMessage(message_id);
}

bool DiscordClient::IsThreadJoined(Snowflake thread_id) const {
    return std::find(m_joined_threads.begin(), m_joined_threads.end(), thread_id) != m_joined_threads.end();
}

bool DiscordClient::HasGuildPermission(Snowflake user_id, Snowflake guild_id, Permission perm) const {
    const auto base = ComputePermissions(user_id, guild_id);
    return (base & perm) == perm;
}

bool DiscordClient::HasSelfChannelPermission(Snowflake channel_id, Permission perm) const {
    return HasChannelPermission(m_user_data.ID, channel_id, perm);
}

bool DiscordClient::HasAnyChannelPermission(Snowflake user_id, Snowflake channel_id, Permission perm) const {
    const auto channel = m_store.GetChannel(channel_id);
    if (!channel.has_value() || !channel->GuildID.has_value()) return false;
    const auto base = ComputePermissions(user_id, *channel->GuildID);
    const auto overwrites = ComputeOverwrites(base, user_id, channel_id);
    return (overwrites & perm) != Permission::NONE;
}

bool DiscordClient::HasChannelPermission(Snowflake user_id, Snowflake channel_id, Permission perm) const {
    const auto channel = m_store.GetChannel(channel_id);
    if (!channel.has_value()) return false;
    if (channel->IsDM()) return true;
    const auto base = ComputePermissions(user_id, *channel->GuildID);
    const auto overwrites = ComputeOverwrites(base, user_id, channel_id);
    return (overwrites & perm) == perm;
}

Permission DiscordClient::ComputePermissions(Snowflake member_id, Snowflake guild_id) const {
    const auto member_roles = m_store.GetMemberRoles(guild_id, member_id);
    const auto guild_owner = m_store.GetGuildOwner(guild_id);

    if (guild_owner == member_id)
        return Permission::ALL;

    const auto everyone = GetRole(guild_id);
    if (!everyone.has_value())
        return Permission::NONE;

    Permission perms = everyone->Permissions;
    for (const auto role_id : member_roles) {
        const auto role = GetRole(role_id);
        if (role.has_value())
            perms |= role->Permissions;
    }

    if ((perms & Permission::ADMINISTRATOR) == Permission::ADMINISTRATOR)
        return Permission::ALL;

    return perms;
}

Permission DiscordClient::ComputeOverwrites(Permission base, Snowflake member_id, Snowflake channel_id) const {
    if ((base & Permission::ADMINISTRATOR) == Permission::ADMINISTRATOR)
        return Permission::ALL;

    const auto channel = GetChannel(channel_id);
    const auto member_roles = m_store.GetMemberRoles(*channel->GuildID, member_id);
    if (!channel.has_value())
        return Permission::NONE;

    Permission perms = base;
    const auto overwrite_everyone = GetPermissionOverwrite(channel_id, *channel->GuildID);
    if (overwrite_everyone.has_value()) {
        perms &= ~overwrite_everyone->Deny;
        perms |= overwrite_everyone->Allow;
    }

    Permission allow = Permission::NONE;
    Permission deny = Permission::NONE;
    for (const auto role_id : member_roles) {
        const auto overwrite = GetPermissionOverwrite(channel_id, role_id);
        if (overwrite.has_value()) {
            allow |= overwrite->Allow;
            deny |= overwrite->Deny;
        }
    }

    perms &= ~deny;
    perms |= allow;

    const auto member_overwrite = GetPermissionOverwrite(channel_id, member_id);
    if (member_overwrite.has_value()) {
        perms &= ~member_overwrite->Deny;
        perms |= member_overwrite->Allow;
    }

    return perms;
}

bool DiscordClient::CanManageMember(Snowflake guild_id, Snowflake actor, Snowflake target) const {
    const auto guild = GetGuild(guild_id);
    if (guild.has_value() && guild->OwnerID == target) return false;
    const auto actor_highest = GetMemberHighestRole(guild_id, actor);
    const auto target_highest = GetMemberHighestRole(guild_id, target);
    if (!actor_highest.has_value()) return false;
    if (!target_highest.has_value()) return true;
    return actor_highest->Position > target_highest->Position;
}

void DiscordClient::ChatMessageCallback(const std::string &nonce, const http::response_type &response, const sigc::slot<void(DiscordError)> &callback) {
    if (!CheckCode(response)) {
        if (response.status_code == http::TooManyRequests) {
            try { // not sure if this body is guaranteed
                RateLimitedResponse r = nlohmann::json::parse(response.text);
                m_signal_message_send_fail.emit(nonce, r.RetryAfter);
            } catch (...) {
                m_signal_message_send_fail.emit(nonce, 0);
            }
        } else {
            m_signal_message_send_fail.emit(nonce, 0);
        }

        // todo actually callback with correct error code (not necessary rn)
        callback(DiscordError::GENERIC);
    } else {
        callback(DiscordError::NONE);
    }
}

void DiscordClient::SendChatMessageNoAttachments(const ChatSubmitParams &params, const sigc::slot<void(DiscordError)> &callback) {
    const auto nonce = std::to_string(Snowflake::FromNow());

    CreateMessageObject obj;
    obj.Content = params.Message;
    obj.Nonce = nonce;
    if (params.Silent) {
        obj.Flags |= MessageFlags::SUPPRESS_NOTIFICATIONS;
    }

    if (params.InReplyToID.IsValid()) {
        obj.MessageReference.emplace().MessageID = params.InReplyToID;
    }

    m_http.MakePOST("/channels/" + std::to_string(params.ChannelID) + "/messages",
                    nlohmann::json(obj).dump(),
                    [this, nonce, callback](const http::response_type &r) {
                        ChatMessageCallback(nonce, r, callback);
                    });

    // dummy preview data
    Message tmp;
    tmp.Content = params.Message;
    tmp.ID = nonce;
    tmp.ChannelID = params.ChannelID;
    tmp.Author = GetUserData();
    tmp.IsTTS = false;
    tmp.DoesMentionEveryone = false;
    tmp.Type = MessageType::DEFAULT;
    tmp.IsPinned = false;
    tmp.Timestamp = "2000-01-01T00:00:00.000000+00:00";
    tmp.Nonce = nonce;
    tmp.IsPending = true;

    m_store.SetMessage(tmp.ID, tmp);
    m_signal_message_create.emit(tmp);
}

void DiscordClient::SendChatMessageAttachments(const ChatSubmitParams &params, const sigc::slot<void(DiscordError)> &callback) {
    const auto nonce = std::to_string(Snowflake::FromNow());

    CreateMessageObject obj;
    obj.Content = params.Message;
    obj.Nonce = nonce;
    obj.Attachments.emplace();
    if (params.Silent) {
        obj.Flags |= MessageFlags::SUPPRESS_NOTIFICATIONS;
    }

    if (params.InReplyToID.IsValid()) {
        obj.MessageReference.emplace().MessageID = params.InReplyToID;
    }

    auto req = m_http.CreateRequest(http::REQUEST_POST, "/channels/" + std::to_string(params.ChannelID) + "/messages");
    m_progress_cb_timer.start();
    req.set_progress_callback([this, nonce](curl_off_t ultotal, curl_off_t ulnow) {
        if (m_progress_cb_timer.elapsed() < 0.0417) return; // try to prevent it from blocking ui
        m_progress_cb_timer.start();
        m_generic_mutex.lock();
        m_generic_queue.push([this, nonce, ultotal, ulnow] {
            m_signal_message_progress.emit(
                nonce,
                static_cast<float>(ulnow) / static_cast<float>(ultotal));
        });
        m_generic_mutex.unlock();
        m_generic_dispatch.emit();
    });
    req.make_form();

    for (size_t i = 0; i < params.Attachments.size(); i++) {
        auto &attachment = params.Attachments.at(i);
        const auto field_name = "files[" + std::to_string(i) + "]";
        req.add_file(field_name, attachment.File, attachment.Filename);
        obj.Attachments->push_back({ static_cast<int>(i), attachment.Description });
    }

    req.add_field("payload_json", nlohmann::json(obj).dump().c_str(), CURL_ZERO_TERMINATED);

    m_http.Execute(std::move(req), [this, params, nonce, callback](const http::response_type &res) {
        for (const auto &attachment : params.Attachments) {
            if (attachment.Type == ChatSubmitParams::AttachmentType::PastedImage) {
                attachment.File->remove();
            }
        }
        ChatMessageCallback(nonce, res, callback);
    });

    // dummy preview data
    Message tmp;
    tmp.Content = params.Message;
    tmp.ID = nonce;
    tmp.ChannelID = params.ChannelID;
    tmp.Author = GetUserData();
    tmp.IsTTS = false;
    tmp.DoesMentionEveryone = false;
    tmp.Type = MessageType::DEFAULT;
    tmp.IsPinned = false;
    tmp.Timestamp = "2000-01-01T00:00:00.000000+00:00";
    tmp.Nonce = nonce;
    tmp.IsPending = true;

    m_store.SetMessage(tmp.ID, tmp);
    m_signal_message_create.emit(tmp);
}

void DiscordClient::SendChatMessage(const ChatSubmitParams &params, const sigc::slot<void(DiscordError)> &callback) {
    if (params.Attachments.empty()) {
        SendChatMessageNoAttachments(params, callback);
    } else {
        SendChatMessageAttachments(params, callback);
    }
}

void DiscordClient::DeleteMessage(Snowflake channel_id, Snowflake id) {
    std::string path = "/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(id);
    m_http.MakeDELETE(path, [](auto) {});
}

void DiscordClient::EditMessage(Snowflake channel_id, Snowflake id, std::string content) {
    std::string path = "/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(id);
    MessageEditObject obj;
    obj.Content = std::move(content);
    nlohmann::json j = obj;
    m_http.MakePATCH(path, j.dump(), [](auto) {});
}

void DiscordClient::SendLazyLoad(Snowflake id) {
    LazyLoadRequestMessage msg;
    msg.Channels.emplace();
    msg.Channels.value()[id] = {
        std::make_pair(0, 99),
        std::make_pair(100, 199)
    };
    msg.GuildID = *GetChannel(id)->GuildID;
    msg.ShouldGetActivities = true;
    msg.ShouldGetTyping = true;
    msg.ShouldGetThreads = true;

    m_websocket.Send(msg);

    m_channels_lazy_loaded.insert(id);
}

void DiscordClient::SendThreadLazyLoad(Snowflake id) {
    auto thread = GetChannel(id);
    if (thread.has_value())
        if (m_channels_lazy_loaded.find(*thread->ParentID) == m_channels_lazy_loaded.end())
            SendLazyLoad(*thread->ParentID);

    LazyLoadRequestMessage msg;
    msg.GuildID = *GetChannel(id)->GuildID;
    msg.ThreadIDs.emplace().push_back(id);

    m_websocket.Send(msg);
}

void DiscordClient::LeaveGuild(Snowflake id) {
    m_http.MakeDELETE("/users/@me/guilds/" + std::to_string(id), [](auto) {});
}

void DiscordClient::KickUser(Snowflake user_id, Snowflake guild_id) {
    m_http.MakeDELETE("/guilds/" + std::to_string(guild_id) + "/members/" + std::to_string(user_id), [](auto) {});
}

void DiscordClient::BanUser(Snowflake user_id, Snowflake guild_id) {
    m_http.MakePUT("/guilds/" + std::to_string(guild_id) + "/bans/" + std::to_string(user_id), "{}", [](auto) {});
}

void DiscordClient::UpdateStatus(PresenceStatus status, bool is_afk) {
    UpdateStatusMessage msg;
    msg.Status = status;
    msg.IsAFK = is_afk;

    m_websocket.Send(nlohmann::json(msg));
    // fake message cuz we dont receive messages for ourself
    m_user_to_status[m_user_data.ID] = status;
    m_signal_presence_update.emit(GetUserData(), status);
}

void DiscordClient::UpdateStatus(PresenceStatus status, bool is_afk, const ActivityData &obj) {
    UpdateStatusMessage msg;
    msg.Status = status;
    msg.IsAFK = is_afk;
    msg.Activities.push_back(obj);

    m_websocket.Send(nlohmann::json(msg));
    m_user_to_status[m_user_data.ID] = status;
    m_signal_presence_update.emit(GetUserData(), status);
}

void DiscordClient::CloseDM(Snowflake channel_id) {
    m_http.MakeDELETE("/channels/" + std::to_string(channel_id), [](const http::response &response) {
        CheckCode(response);
    });
}

std::optional<Snowflake> DiscordClient::FindDM(Snowflake user_id) {
    const auto &channels = m_store.GetChannels();
    for (const auto &id : channels) {
        const auto channel = m_store.GetChannel(id);
        const auto recipients = channel->GetDMRecipients();
        if (channel->Type == ChannelType::DM && recipients.size() == 1 && recipients[0].ID == user_id)
            return id;
    }

    return std::nullopt;
}

void DiscordClient::AddReaction(Snowflake id, Glib::ustring param) {
    if (!param.is_ascii()) // means unicode param
        param = Glib::uri_escape_string(param, "", false);
    else {
        const auto &tmp = m_store.GetEmoji(param);
        if (tmp.has_value())
            param = tmp->Name + ":" + std::to_string(tmp->ID);
        else
            return;
    }
    Snowflake channel_id = m_store.GetMessage(id)->ChannelID;
    m_http.MakePUT("/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(id) + "/reactions/" + param + "/@me", "", [](auto) {});
}

void DiscordClient::RemoveReaction(Snowflake id, Glib::ustring param) {
    if (!param.is_ascii()) // means unicode param
        param = Glib::uri_escape_string(param, "", false);
    else {
        const auto &tmp = m_store.GetEmoji(param);
        if (tmp.has_value())
            param = tmp->Name + ":" + std::to_string(tmp->ID);
        else
            return;
    }
    Snowflake channel_id = m_store.GetMessage(id)->ChannelID;
    m_http.MakeDELETE("/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(id) + "/reactions/" + param + "/@me", [](auto) {});
}

void DiscordClient::SetGuildName(Snowflake id, const Glib::ustring &name, const sigc::slot<void(DiscordError code)> &callback) {
    ModifyGuildObject obj;
    obj.Name = name;
    m_http.MakePATCH("/guilds/" + std::to_string(id), nlohmann::json(obj).dump(), [callback](const http::response_type &r) {
        if (CheckCode(r))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(r));
    });
}

void DiscordClient::SetGuildIcon(Snowflake id, const std::string &data, const sigc::slot<void(DiscordError code)> &callback) {
    ModifyGuildObject obj;
    obj.IconData = data;
    m_http.MakePATCH("/guilds/" + std::to_string(id), nlohmann::json(obj).dump(), [callback](const http::response_type &r) {
        if (CheckCode(r))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(r));
    });
}

void DiscordClient::UnbanUser(Snowflake guild_id, Snowflake user_id, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakeDELETE("/guilds/" + std::to_string(guild_id) + "/bans/" + std::to_string(user_id), [callback](const http::response_type &response) {
        if (CheckCode(response, 204))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::DeleteInvite(const std::string &code, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakeDELETE("/invites/" + code, [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::AddGroupDMRecipient(Snowflake channel_id, Snowflake user_id) {
    m_http.MakePUT("/channels/" + std::to_string(channel_id) + "/recipients/" + std::to_string(user_id), "", [](const http::response_type &response) {
        CheckCode(response);
    });
}

void DiscordClient::RemoveGroupDMRecipient(Snowflake channel_id, Snowflake user_id) {
    m_http.MakeDELETE("/channels/" + std::to_string(channel_id) + "/recipients/" + std::to_string(user_id), [](const http::response_type &response) {
        CheckCode(response);
    });
}

void DiscordClient::ModifyRolePermissions(Snowflake guild_id, Snowflake role_id, Permission permissions, const sigc::slot<void(DiscordError code)> &callback) {
    ModifyGuildRoleObject obj;
    obj.Permissions = permissions;
    m_http.MakePATCH("/guilds/" + std::to_string(guild_id) + "/roles/" + std::to_string(role_id), nlohmann::json(obj).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::ModifyRoleName(Snowflake guild_id, Snowflake role_id, const Glib::ustring &name, const sigc::slot<void(DiscordError code)> &callback) {
    ModifyGuildRoleObject obj;
    obj.Name = name;
    m_http.MakePATCH("/guilds/" + std::to_string(guild_id) + "/roles/" + std::to_string(role_id), nlohmann::json(obj).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::ModifyRoleColor(Snowflake guild_id, Snowflake role_id, uint32_t color, const sigc::slot<void(DiscordError code)> &callback) {
    ModifyGuildRoleObject obj;
    obj.Color = color;
    m_http.MakePATCH("/guilds/" + std::to_string(guild_id) + "/roles/" + std::to_string(role_id), nlohmann::json(obj).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::ModifyRoleColor(Snowflake guild_id, Snowflake role_id, const Gdk::RGBA &color, const sigc::slot<void(DiscordError code)> &callback) {
    uint32_t int_color = 0;
    int_color |= static_cast<uint32_t>(color.get_blue() * 255.0) << 0;
    int_color |= static_cast<uint32_t>(color.get_green() * 255.0) << 8;
    int_color |= static_cast<uint32_t>(color.get_red() * 255.0) << 16;
    ModifyRoleColor(guild_id, role_id, int_color, callback);
}

void DiscordClient::ModifyRolePosition(Snowflake guild_id, Snowflake role_id, int position, const sigc::slot<void(DiscordError code)> &callback) {
    const auto guild = GetGuild(guild_id);
    if (!guild.has_value() || !guild->Roles.has_value()) return;
    const auto &roles = *guild->Roles;
    if (static_cast<size_t>(position) > roles.size()) return;
    // gay and makes you send every role in between new and old position
    constexpr auto IDX_MAX = ~size_t { 0 };
    size_t index_from = IDX_MAX, index_to = IDX_MAX;
    for (size_t i = 0; i < roles.size(); i++) {
        const auto &role = roles[i];
        if (role.ID == role_id)
            index_from = i;
        else if (role.Position == position)
            index_to = i;
        if (index_from != IDX_MAX && index_to != IDX_MAX) break;
    }

    if (index_from == IDX_MAX || index_to == IDX_MAX) return;

    int dir;
    size_t range_from, range_to;
    if (index_to > index_from) {
        dir = 1;
        range_from = index_from + 1;
        range_to = index_to + 1;
    } else {
        dir = -1;
        range_from = index_to;
        range_to = index_from;
    }

    ModifyGuildRolePositionsObject obj;

    obj.Positions.push_back({ roles[index_from].ID, position });
    for (size_t i = range_from; i < range_to; i++)
        obj.Positions.push_back({ roles[i].ID, roles[i].Position + dir });

    m_http.MakePATCH("/guilds/" + std::to_string(guild_id) + "/roles", nlohmann::json(obj).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::ModifyEmojiName(Snowflake guild_id, Snowflake emoji_id, const Glib::ustring &name, const sigc::slot<void(DiscordError code)> &callback) {
    ModifyGuildEmojiObject obj;
    obj.Name = name;

    m_http.MakePATCH("/guilds/" + std::to_string(guild_id) + "/emojis/" + std::to_string(emoji_id), nlohmann::json(obj).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::DeleteEmoji(Snowflake guild_id, Snowflake emoji_id, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakeDELETE("/guilds/" + std::to_string(guild_id) + "/emojis/" + std::to_string(emoji_id), [callback](const http::response_type &response) {
        if (CheckCode(response, 204))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::RemoveRelationship(Snowflake id, const sigc::slot<void(DiscordError Code)> &callback) {
    m_http.MakeDELETE("/users/@me/relationships/" + std::to_string(id), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::SendFriendRequest(const Glib::ustring &username, int discriminator, const sigc::slot<void(DiscordError code)> &callback) {
    FriendRequestObject obj;
    obj.Username = username;
    obj.Discriminator = discriminator;
    m_http.MakePOST("/users/@me/relationships", nlohmann::json(obj).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response, 204))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::PutRelationship(Snowflake id, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakePUT("/users/@me/relationships/" + std::to_string(id), "{}", [callback](const http::response_type &response) {
        if (CheckCode(response, 204))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::Pin(Snowflake channel_id, Snowflake message_id, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakePUT("/channels/" + std::to_string(channel_id) + "/pins/" + std::to_string(message_id), "", [callback](const http::response_type &response) {
        if (CheckCode(response, 204))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::Unpin(Snowflake channel_id, Snowflake message_id, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakeDELETE("/channels/" + std::to_string(channel_id) + "/pins/" + std::to_string(message_id), [callback](const http::response_type &response) {
        if (CheckCode(response, 204))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

// i dont know if the location parameter is necessary at all but discord's thread implementation is extremely strange
// so its here just in case
void DiscordClient::LeaveThread(Snowflake channel_id, const std::string &location, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakeDELETE("/channels/" + std::to_string(channel_id) + "/thread-members/@me?location=" + location, [callback](const http::response_type &response) {
        if (CheckCode(response, 204))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::ArchiveThread(Snowflake channel_id, const sigc::slot<void(DiscordError code)> &callback) {
    ModifyChannelObject obj;
    obj.Archived = true;
    obj.Locked = true;
    m_http.MakePATCH("/channels/" + std::to_string(channel_id), nlohmann::json(obj).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::UnArchiveThread(Snowflake channel_id, const sigc::slot<void(DiscordError code)> &callback) {
    ModifyChannelObject obj;
    obj.Archived = false;
    obj.Locked = false;
    m_http.MakePATCH("/channels/" + std::to_string(channel_id), nlohmann::json(obj).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::MarkChannelAsRead(Snowflake channel_id, const sigc::slot<void(DiscordError code)> &callback) {
    if (m_unread.find(channel_id) == m_unread.end()) return;
    const auto iter = m_last_message_id.find(channel_id);
    if (iter == m_last_message_id.end()) return;
    m_http.MakePOST("/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(iter->second) + "/ack", "{\"token\":null}", [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::MarkGuildAsRead(Snowflake guild_id, const sigc::slot<void(DiscordError code)> &callback) {
    AckBulkData data;
    const auto channels = GetChannelsInGuild(guild_id);
    for (const auto &[unread, mention_count] : m_unread) {
        if (channels.find(unread) == channels.end()) continue;

        const auto iter = m_last_message_id.find(unread);
        if (iter == m_last_message_id.end()) continue;
        auto &e = data.ReadStates.emplace_back();
        e.ID = unread;
        e.LastMessageID = iter->second;
    }

    if (data.ReadStates.empty()) return;

    m_http.MakePOST("/read-states/ack-bulk", nlohmann::json(data).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::MuteChannel(Snowflake channel_id, const sigc::slot<void(DiscordError code)> &callback) {
    const auto channel = GetChannel(channel_id);
    if (!channel.has_value()) return;
    const auto guild_id_path = channel->GuildID.has_value() ? std::to_string(*channel->GuildID) : "@me"s;
    nlohmann::json j;
    j["channel_overrides"][std::to_string(channel_id)]["mute_config"] = MuteConfigData { std::nullopt, -1 };
    j["channel_overrides"][std::to_string(channel_id)]["muted"] = true;
    m_http.MakePATCH("/users/@me/guilds/" + guild_id_path + "/settings", j.dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::UnmuteChannel(Snowflake channel_id, const sigc::slot<void(DiscordError code)> &callback) {
    const auto channel = GetChannel(channel_id);
    if (!channel.has_value()) return;
    const auto guild_id_path = channel->GuildID.has_value() ? std::to_string(*channel->GuildID) : "@me"s;
    nlohmann::json j;
    j["channel_overrides"][std::to_string(channel_id)]["muted"] = false;
    m_http.MakePATCH("/users/@me/guilds/" + guild_id_path + "/settings", j.dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::MuteGuild(Snowflake id, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakePATCH("/users/@me/guilds/" + std::to_string(id) + "/settings", R"({"muted":true})", [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::UnmuteGuild(Snowflake id, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakePATCH("/users/@me/guilds/" + std::to_string(id) + "/settings", R"({"muted":false})", [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::MuteThread(Snowflake id, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakePATCH("/channels/" + std::to_string(id) + "/thread-members/@me/settings", R"({"muted":true})", [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::UnmuteThread(Snowflake id, const sigc::slot<void(DiscordError code)> &callback) {
    m_http.MakePATCH("/channels/" + std::to_string(id) + "/thread-members/@me/settings", R"({"muted":false})", [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::FetchPinned(Snowflake id, const sigc::slot<void(std::vector<Message>, DiscordError code)> &callback) {
    // return from db if we know the pins have already been requested
    if (m_channels_pinned_requested.find(id) != m_channels_pinned_requested.end()) {
        callback(m_store.GetPinnedMessages(id), DiscordError::NONE);
        return;
    }
    m_channels_pinned_requested.insert(id);

    m_http.MakeGET("/channels/" + std::to_string(id) + "/pins", [this, callback](const http::response_type &response) {
        if (!CheckCode(response)) {
            callback({}, GetCodeFromResponse(response));
            return;
        }

        auto data = nlohmann::json::parse(response.text).get<std::vector<Message>>();
        std::sort(data.begin(), data.end(), [](const Message &a, const Message &b) { return a.ID < b.ID; });
        for (auto &msg : data)
            StoreMessageData(msg);
        callback(data, DiscordError::NONE);
    });
}

bool DiscordClient::CanModifyRole(Snowflake guild_id, Snowflake role_id, Snowflake user_id) const {
    const auto guild = *GetGuild(guild_id);
    if (guild.OwnerID == user_id) return true;
    const auto role = *GetRole(role_id);
    const auto has_modify = HasGuildPermission(user_id, guild_id, Permission::MANAGE_CHANNELS);
    const auto highest = GetMemberHighestRole(guild_id, user_id);
    return has_modify && highest.has_value() && highest->Position > role.Position;
}

bool DiscordClient::CanModifyRole(Snowflake guild_id, Snowflake role_id) const {
    return CanModifyRole(guild_id, role_id, GetUserData().ID);
}

std::vector<BanData> DiscordClient::GetBansInGuild(Snowflake guild_id) {
    return m_store.GetBans(guild_id);
}

void DiscordClient::FetchGuildBan(Snowflake guild_id, Snowflake user_id, const sigc::slot<void(BanData)> &callback) {
    m_http.MakeGET("/guilds/" + std::to_string(guild_id) + "/bans/" + std::to_string(user_id), [this, callback, guild_id](const http::response_type &response) {
        if (!CheckCode(response)) return;
        auto ban = nlohmann::json::parse(response.text).get<BanData>();
        m_store.SetBan(guild_id, ban.User.ID, ban);
        m_store.SetUser(ban.User.ID, ban.User);
        callback(ban);
    });
}

void DiscordClient::FetchGuildBans(Snowflake guild_id, const sigc::slot<void(std::vector<BanData>)> &callback) {
    m_http.MakeGET("/guilds/" + std::to_string(guild_id) + "/bans", [this, callback, guild_id](const http::response_type &response) {
        if (!CheckCode(response)) return;
        auto bans = nlohmann::json::parse(response.text).get<std::vector<BanData>>();
        m_store.BeginTransaction();
        for (const auto &ban : bans) {
            m_store.SetBan(guild_id, ban.User.ID, ban);
            m_store.SetUser(ban.User.ID, ban.User);
        }
        m_store.EndTransaction();
        callback(bans);
    });
}

void DiscordClient::FetchGuildInvites(Snowflake guild_id, const sigc::slot<void(std::vector<InviteData>)> &callback) {
    m_http.MakeGET("/guilds/" + std::to_string(guild_id) + "/invites", [this, callback](const http::response_type &response) {
        // store?
        if (!CheckCode(response)) return;
        auto invites = nlohmann::json::parse(response.text).get<std::vector<InviteData>>();

        m_store.BeginTransaction();
        for (const auto &invite : invites)
            if (invite.Inviter.has_value())
                m_store.SetUser(invite.Inviter->ID, *invite.Inviter);
        m_store.EndTransaction();

        callback(invites);
    });
}

void DiscordClient::FetchAuditLog(Snowflake guild_id, const sigc::slot<void(AuditLogData)> &callback) {
    m_http.MakeGET("/guilds/" + std::to_string(guild_id) + "/audit-logs", [this, callback](const http::response &response) {
        if (!CheckCode(response)) return;
        auto data = nlohmann::json::parse(response.text).get<AuditLogData>();

        m_store.BeginTransaction();
        for (const auto &user : data.Users)
            m_store.SetUser(user.ID, user);
        m_store.EndTransaction();

        callback(data);
    });
}

void DiscordClient::FetchGuildEmojis(Snowflake guild_id, const sigc::slot<void(std::vector<EmojiData>)> &callback) {
    m_http.MakeGET("/guilds/" + std::to_string(guild_id) + "/emojis", [this, callback](const http::response_type &response) {
        if (!CheckCode(response)) return;
        auto emojis = nlohmann::json::parse(response.text).get<std::vector<EmojiData>>();
        m_store.BeginTransaction();
        for (const auto &emoji : emojis)
            m_store.SetEmoji(emoji.ID, emoji);
        m_store.EndTransaction();
        callback(emojis);
    });
}

void DiscordClient::FetchUserProfile(Snowflake user_id, const sigc::slot<void(UserProfileData)> &callback) {
    m_http.MakeGET("/users/" + std::to_string(user_id) + "/profile", [callback](const http::response_type &response) {
        if (!CheckCode(response)) return;
        callback(nlohmann::json::parse(response.text).get<UserProfileData>());
    });
}

void DiscordClient::FetchUserNote(Snowflake user_id, const sigc::slot<void(std::string note)> &callback) {
    m_http.MakeGET("/users/@me/notes/" + std::to_string(user_id), [callback](const http::response_type &response) {
        if (response.status_code == 404) return;
        if (!CheckCode(response)) return;
        const auto note = nlohmann::json::parse(response.text).get<UserNoteObject>().Note;
        if (note.has_value())
            callback(*note);
    });
}

void DiscordClient::SetUserNote(Snowflake user_id, std::string note, const sigc::slot<void(DiscordError code)> &callback) {
    UserSetNoteObject obj;
    obj.Note = std::move(note);
    m_http.MakePUT("/users/@me/notes/" + std::to_string(user_id), nlohmann::json(obj).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response, 204))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::FetchUserRelationships(Snowflake user_id, const sigc::slot<void(std::vector<UserData>)> &callback) {
    m_http.MakeGET("/users/" + std::to_string(user_id) + "/relationships", [this, callback](const http::response_type &response) {
        if (!CheckCode(response)) return;
        RelationshipsData data = nlohmann::json::parse(response.text);
        for (const auto &user : data.Users)
            m_store.SetUser(user.ID, user);
        callback(data.Users);
    });
}

bool DiscordClient::IsVerificationRequired(Snowflake guild_id) const {
    const auto member = GetMember(GetUserData().ID, guild_id);
    if (member.has_value() && member->IsPending.has_value())
        return *member->IsPending;
    return false;
}

void DiscordClient::GetVerificationGateInfo(Snowflake guild_id, const sigc::slot<void(std::optional<VerificationGateInfoObject>)> &callback) {
    m_http.MakeGET("/guilds/" + std::to_string(guild_id) + "/member-verification", [callback](const http::response_type &response) {
        if (!CheckCode(response)) return;
        if (response.status_code == 204) callback(std::nullopt);
        callback(nlohmann::json::parse(response.text).get<VerificationGateInfoObject>());
    });
}

void DiscordClient::AcceptVerificationGate(Snowflake guild_id, VerificationGateInfoObject info, const sigc::slot<void(DiscordError code)> &callback) {
    if (info.VerificationFields.has_value())
        for (auto &field : *info.VerificationFields)
            field.Response = true;
    m_http.MakePUT("/guilds/" + std::to_string(guild_id) + "/requests/@me", nlohmann::json(info).dump(), [callback](const http::response_type &response) {
        if (CheckCode(response))
            callback(DiscordError::NONE);
        else
            callback(GetCodeFromResponse(response));
    });
}

void DiscordClient::RemoteAuthLogin(const std::string &ticket, const sigc::slot<void(std::optional<std::string>, DiscordError code)> &callback) {
    http::request req(http::REQUEST_POST, "https://discord.com/api/v9/users/@me/remote-auth/login");
    req.set_header("Content-Type", "application/json");
    req.set_user_agent(Abaddon::Get().GetSettings().UserAgent);
    req.set_body("{\"ticket\":\"" + ticket + "\"}");
    m_http.Execute(std::move(req), [this, callback](const http::response_type &r) {
        if (CheckCode(r)) {
            callback(nlohmann::json::parse(r.text).at("encrypted_token").get<std::string>(), DiscordError::NONE);
        } else {
            try {
                const auto j = nlohmann::json::parse(r.text);
                if (j.contains("captcha_service")) {
                    callback(std::nullopt, DiscordError::CAPTCHA_REQUIRED);
                    return;
                }
            } catch (...) {}
            callback(std::nullopt, GetCodeFromResponse(r));
        }
    });
}

#ifdef WITH_VOICE
void DiscordClient::ConnectToVoice(Snowflake channel_id) {
    auto channel = GetChannel(channel_id);
    if (!channel.has_value()) return;
    m_voice_channel_id = channel_id;
    VoiceStateUpdateMessage m;
    if (channel->GuildID.has_value())
        m.GuildID = channel->GuildID;
    m.ChannelID = channel_id;
    m.PreferredRegion = "newark";
    m_websocket.Send(m);
    m_signal_voice_requested_connect.emit(channel_id);
}

void DiscordClient::DisconnectFromVoice() {
    m_voice.Stop();
    VoiceStateUpdateMessage m;
    m_websocket.Send(m);
    m_signal_voice_requested_disconnect.emit();
}

bool DiscordClient::IsVoiceConnected() const noexcept {
    return m_voice.IsConnected();
}

bool DiscordClient::IsVoiceConnecting() const noexcept {
    return m_voice.IsConnecting();
}

Snowflake DiscordClient::GetVoiceChannelID() const noexcept {
    return m_voice_channel_id;
}

std::unordered_set<Snowflake> DiscordClient::GetUsersInVoiceChannel(Snowflake channel_id) {
    return m_voice_state_channel_users[channel_id];
}

std::optional<uint32_t> DiscordClient::GetSSRCOfUser(Snowflake id) const {
    return m_voice.GetSSRCOfUser(id);
}

std::optional<std::pair<Snowflake, VoiceStateFlags>> DiscordClient::GetVoiceState(Snowflake user_id) const {
    if (const auto it = m_voice_states.find(user_id); it != m_voice_states.end()) {
        return it->second;
    }
    return std::nullopt;
}

DiscordVoiceClient &DiscordClient::GetVoiceClient() {
    return m_voice;
}

void DiscordClient::SetVoiceMuted(bool is_mute) {
    m_mute_requested = is_mute;
    SendVoiceStateUpdate();
}

void DiscordClient::SetVoiceDeafened(bool is_deaf) {
    m_deaf_requested = is_deaf;
    SendVoiceStateUpdate();
}
#endif

void DiscordClient::SetReferringChannel(Snowflake id) {
    if (!id.IsValid()) {
        m_http.SetPersistentHeader("Referer", "https://discord.com/channels/@me");
    } else {
        const auto channel = GetChannel(id);
        if (channel.has_value()) {
            if (channel->IsDM()) {
                m_http.SetPersistentHeader("Referer", "https://discord.com/channels/@me/" + std::to_string(id));
            } else if (channel->GuildID.has_value()) {
                m_http.SetPersistentHeader("Referer", "https://discord.com/channels/" + std::to_string(*channel->GuildID) + "/" + std::to_string(id));
            } else {
                m_http.SetPersistentHeader("Referer", "https://discord.com/channels/@me");
            }
        } else {
            m_http.SetPersistentHeader("Referer", "https://discord.com/channels/@me");
        }
    }
}

void DiscordClient::SetBuildNumber(uint32_t build_number) {
    m_build_number = build_number;
}

void DiscordClient::SetCookie(std::string_view cookie) {
    m_http.SetCookie(cookie);
}

void DiscordClient::UpdateToken(const std::string &token) {
    if (!IsStarted()) {
        m_token = token;
        m_http.SetAuth(token);
    }
}

void DiscordClient::SetUserAgent(const std::string &agent) {
    m_http.SetUserAgent(agent);
    m_websocket.SetUserAgent(agent);
}

void DiscordClient::SetDumpReady(bool dump) {
    m_dump_ready = dump;
}

bool DiscordClient::IsChannelMuted(Snowflake id) const noexcept {
    return m_muted_channels.find(id) != m_muted_channels.end();
}

bool DiscordClient::IsGuildMuted(Snowflake id) const noexcept {
    return m_muted_guilds.find(id) != m_muted_guilds.end();
}

int DiscordClient::GetUnreadStateForChannel(Snowflake id) const noexcept {
    const auto iter = m_unread.find(id);
    if (iter == m_unread.end()) return -1; // todo: no magic number (who am i kidding ill never change this)
    return iter->second;
}

int DiscordClient::GetUnreadChannelsCountForCategory(Snowflake id) const noexcept {
    int result = 0;
    for (Snowflake channel_id : m_store.GetChannelIDsWithParentID(id)) {
        if (IsChannelMuted(channel_id)) continue;
        const auto iter = m_unread.find(channel_id);
        if (iter == m_unread.end()) continue;
        result += 1;
    }
    return result;
}

bool DiscordClient::GetUnreadStateForGuild(Snowflake id, int &total_mentions) const noexcept {
    total_mentions = 0;
    bool has_any_unread = false;
    const auto channels = GetChannelsInGuild(id);
    for (const auto channel_id : channels) {
        const auto channel_unread = GetUnreadStateForChannel(channel_id);
        if (channel_unread > -1)
            total_mentions += channel_unread;

        // channels under muted categories wont contribute to unread state
        if (const auto iter = m_channel_muted_parent.find(channel_id); iter != m_channel_muted_parent.end())
            continue;

        if (!has_any_unread && channel_unread > -1 && !IsChannelMuted(channel_id))
            has_any_unread = true;
    }
    return has_any_unread;
}

int DiscordClient::GetUnreadDMsCount() const {
    const auto channels = GetPrivateChannels();
    int count = 0;
    for (const auto channel_id : channels)
        if (!IsChannelMuted(channel_id) && GetUnreadStateForChannel(channel_id) > -1) count++;
    return count;
}

PresenceStatus DiscordClient::GetUserStatus(Snowflake id) const {
    auto it = m_user_to_status.find(id);
    if (it != m_user_to_status.end())
        return it->second;

    return PresenceStatus::Offline;
}

std::map<Snowflake, RelationshipType> DiscordClient::GetRelationships() const {
    return m_user_relationships;
}

std::set<Snowflake> DiscordClient::GetRelationships(RelationshipType type) const {
    std::set<Snowflake> ret;
    for (const auto &[id, rtype] : m_user_relationships)
        if (rtype == type)
            ret.insert(id);
    return ret;
}

std::optional<RelationshipType> DiscordClient::GetRelationship(Snowflake id) const {
    if (auto it = m_user_relationships.find(id); it != m_user_relationships.end())
        return it->second;
    return std::nullopt;
}

void DiscordClient::HandleGatewayMessageRaw(std::string str) {
    // handles multiple zlib compressed messages, calling HandleGatewayMessage when a full message is received
    std::vector<uint8_t> buf(str.begin(), str.end());
    int len = static_cast<int>(buf.size());
    bool has_suffix = buf[len - 4] == 0x00 && buf[len - 3] == 0x00 && buf[len - 2] == 0xFF && buf[len - 1] == 0xFF;

    m_compressed_buf.insert(m_compressed_buf.end(), buf.begin(), buf.end());

    if (!has_suffix) return;

    m_zstream.next_in = m_compressed_buf.data();
    m_zstream.avail_in = static_cast<uInt>(m_compressed_buf.size());
    m_zstream.total_in = m_zstream.total_out = 0;

    // loop in case of really big messages (e.g. READY)
    while (true) {
        m_zstream.next_out = m_decompress_buf.data() + m_zstream.total_out;
        m_zstream.avail_out = static_cast<uInt>(m_decompress_buf.size() - m_zstream.total_out);

        int err = inflate(&m_zstream, Z_SYNC_FLUSH);
        if ((err == Z_OK || err == Z_BUF_ERROR) && m_zstream.avail_in > 0) {
            m_decompress_buf.resize(m_decompress_buf.size() + InflateChunkSize);
        } else {
            if (err != Z_OK) {
                fprintf(stderr, "Error decompressing input buffer %d (%d/%d)\n", err, m_zstream.avail_in, m_zstream.avail_out);
            } else {
                m_msg_mutex.lock();
                m_msg_queue.push(std::string(m_decompress_buf.begin(), m_decompress_buf.begin() + m_zstream.total_out));
                m_msg_dispatch.emit();
                m_msg_mutex.unlock();
                if (m_decompress_buf.size() > InflateChunkSize)
                    m_decompress_buf.resize(InflateChunkSize);
            }
            break;
        }
    }

    m_compressed_buf.clear();
}

void DiscordClient::MessageDispatch() {
    m_msg_mutex.lock();
    auto msg = m_msg_queue.front();
    m_msg_queue.pop();
    m_msg_mutex.unlock();
    HandleGatewayMessage(msg);
}

void DiscordClient::HandleGatewayMessage(std::string str) {
    GatewayMessage m;
    try {
        m = nlohmann::json::parse(str);
    } catch (std::exception &e) {
        printf("Error decoding JSON. Discarding message: %s\n", e.what());
        return;
    }

    if (m.Sequence != -1)
        m_last_sequence = m.Sequence;

    try {
        switch (m.Opcode) {
            case GatewayOp::Hello: {
                HandleGatewayHello(m);
            } break;
            case GatewayOp::HeartbeatAck: {
                m_heartbeat_acked = true;
            } break;
            case GatewayOp::Reconnect: {
                HandleGatewayReconnect(m);
            } break;
            case GatewayOp::InvalidSession: {
                HandleGatewayInvalidSession(m);
            } break;
            case GatewayOp::Dispatch: {
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
                    case GatewayEvent::MESSAGE_DELETE: {
                        HandleGatewayMessageDelete(m);
                    } break;
                    case GatewayEvent::MESSAGE_UPDATE: {
                        HandleGatewayMessageUpdate(m);
                    } break;
                    case GatewayEvent::GUILD_MEMBER_LIST_UPDATE: {
                        HandleGatewayGuildMemberListUpdate(m);
                    } break;
                    case GatewayEvent::GUILD_CREATE: {
                        HandleGatewayGuildCreate(m);
                    } break;
                    case GatewayEvent::GUILD_DELETE: {
                        HandleGatewayGuildDelete(m);
                    } break;
                    case GatewayEvent::MESSAGE_DELETE_BULK: {
                        HandleGatewayMessageDeleteBulk(m);
                    } break;
                    case GatewayEvent::GUILD_MEMBER_UPDATE: {
                        HandleGatewayGuildMemberUpdate(m);
                    } break;
                    case GatewayEvent::PRESENCE_UPDATE: {
                        HandleGatewayPresenceUpdate(m);
                    } break;
                    case GatewayEvent::CHANNEL_DELETE: {
                        HandleGatewayChannelDelete(m);
                    } break;
                    case GatewayEvent::CHANNEL_UPDATE: {
                        HandleGatewayChannelUpdate(m);
                    } break;
                    case GatewayEvent::CHANNEL_CREATE: {
                        HandleGatewayChannelCreate(m);
                    } break;
                    case GatewayEvent::GUILD_UPDATE: {
                        HandleGatewayGuildUpdate(m);
                    } break;
                    case GatewayEvent::GUILD_ROLE_UPDATE: {
                        HandleGatewayGuildRoleUpdate(m);
                    } break;
                    case GatewayEvent::GUILD_ROLE_CREATE: {
                        HandleGatewayGuildRoleCreate(m);
                    } break;
                    case GatewayEvent::GUILD_ROLE_DELETE: {
                        HandleGatewayGuildRoleDelete(m);
                    } break;
                    case GatewayEvent::MESSAGE_REACTION_ADD: {
                        HandleGatewayMessageReactionAdd(m);
                    } break;
                    case GatewayEvent::MESSAGE_REACTION_REMOVE: {
                        HandleGatewayMessageReactionRemove(m);
                    } break;
                    case GatewayEvent::CHANNEL_RECIPIENT_ADD: {
                        HandleGatewayChannelRecipientAdd(m);
                    } break;
                    case GatewayEvent::CHANNEL_RECIPIENT_REMOVE: {
                        HandleGatewayChannelRecipientRemove(m);
                    } break;
                    case GatewayEvent::TYPING_START: {
                        HandleGatewayTypingStart(m);
                    } break;
                    case GatewayEvent::GUILD_BAN_REMOVE: {
                        HandleGatewayGuildBanRemove(m);
                    } break;
                    case GatewayEvent::GUILD_BAN_ADD: {
                        HandleGatewayGuildBanAdd(m);
                    } break;
                    case GatewayEvent::INVITE_CREATE: {
                        HandleGatewayInviteCreate(m);
                    } break;
                    case GatewayEvent::INVITE_DELETE: {
                        HandleGatewayInviteDelete(m);
                    } break;
                    case GatewayEvent::USER_NOTE_UPDATE: {
                        HandleGatewayUserNoteUpdate(m);
                    } break;
                    case GatewayEvent::READY_SUPPLEMENTAL: {
                        HandleGatewayReadySupplemental(m);
                    } break;
                    case GatewayEvent::GUILD_EMOJIS_UPDATE: {
                        HandleGatewayGuildEmojisUpdate(m);
                    } break;
                    case GatewayEvent::GUILD_JOIN_REQUEST_CREATE: {
                        HandleGatewayGuildJoinRequestCreate(m);
                    } break;
                    case GatewayEvent::GUILD_JOIN_REQUEST_UPDATE: {
                        HandleGatewayGuildJoinRequestUpdate(m);
                    } break;
                    case GatewayEvent::GUILD_JOIN_REQUEST_DELETE: {
                        HandleGatewayGuildJoinRequestDelete(m);
                    } break;
                    case GatewayEvent::RELATIONSHIP_REMOVE: {
                        HandleGatewayRelationshipRemove(m);
                    } break;
                    case GatewayEvent::RELATIONSHIP_ADD: {
                        HandleGatewayRelationshipAdd(m);
                    } break;
                    case GatewayEvent::THREAD_CREATE: {
                        HandleGatewayThreadCreate(m);
                    } break;
                    case GatewayEvent::THREAD_DELETE: {
                        HandleGatewayThreadDelete(m);
                    } break;
                    case GatewayEvent::THREAD_LIST_SYNC: {
                        HandleGatewayThreadListSync(m);
                    } break;
                    case GatewayEvent::THREAD_MEMBERS_UPDATE: {
                        HandleGatewayThreadMembersUpdate(m);
                    } break;
                    case GatewayEvent::THREAD_MEMBER_UPDATE: {
                        HandleGatewayThreadMemberUpdate(m);
                    } break;
                    case GatewayEvent::THREAD_UPDATE: {
                        HandleGatewayThreadUpdate(m);
                    } break;
                    case GatewayEvent::THREAD_MEMBER_LIST_UPDATE: {
                        HandleGatewayThreadMemberListUpdate(m);
                    } break;
                    case GatewayEvent::MESSAGE_ACK: {
                        HandleGatewayMessageAck(m);
                    } break;
                    case GatewayEvent::USER_GUILD_SETTINGS_UPDATE: {
                        HandleGatewayUserGuildSettingsUpdate(m);
                    } break;
                    case GatewayEvent::GUILD_MEMBERS_CHUNK: {
                        HandleGatewayGuildMembersChunk(m);
                    } break;
#ifdef WITH_VOICE
                    case GatewayEvent::VOICE_STATE_UPDATE: {
                        HandleGatewayVoiceStateUpdate(m);
                    } break;
                    case GatewayEvent::VOICE_SERVER_UPDATE: {
                        HandleGatewayVoiceServerUpdate(m);
                    } break;
                    case GatewayEvent::CALL_CREATE: {
                        HandleGatewayCallCreate(m);
                    } break;
#endif
                }
            } break;
            default:
                printf("Unknown opcode %d\n", static_cast<int>(m.Opcode));
                break;
        }
    } catch (std::exception &e) {
        fprintf(stderr, "error handling message (opcode %d): %s\n", static_cast<int>(m.Opcode), e.what());
    }
}

void DiscordClient::HandleGatewayHello(const GatewayMessage &msg) {
    m_client_connected = true;
    HelloMessageData d = msg.Data;
    m_heartbeat_msec = d.HeartbeatInterval;
    m_heartbeat_waiter.revive();
    m_heartbeat_thread = std::thread([this] { HeartbeatThread(); });
    m_signal_connected.emit(); // socket is connected before this but emitting here should b fine
    m_reconnecting = false;    // maybe should go elsewhere?
    if (m_wants_resume) {
        m_wants_resume = false;
        SendResume();
    } else
        SendIdentify();
}

// perhaps this should be set by the main class
std::string DiscordClient::GetAPIURL() {
    return Abaddon::Get().GetSettings().APIBaseURL;
}

std::string DiscordClient::GetGatewayURL() {
    return Abaddon::Get().GetSettings().GatewayURL;
}

DiscordError DiscordClient::GetCodeFromResponse(const http::response_type &response) {
    try {
        const auto data = nlohmann::json::parse(response.text);
        return data.at("code").get<DiscordError>();
    } catch (...) {}
    return DiscordError::GENERIC;
}

void DiscordClient::ProcessNewGuild(GuildData &guild) {
    if (guild.IsUnavailable) {
        printf("guild (%" PRIu64 ") unavailable\n", static_cast<uint64_t>(guild.ID));
        return;
    }

    m_store.BeginTransaction();

    m_store.SetGuild(guild.ID, guild);
    if (guild.Channels.has_value()) {
        for (auto &c : *guild.Channels) {
            c.GuildID = guild.ID;
            m_store.SetChannel(c.ID, c);
            m_guild_to_channels[guild.ID].insert(c.ID);
            for (auto &p : *c.PermissionOverwrites) {
                m_store.SetPermissionOverwrite(c.ID, p.ID, p);
            }
        }
    }

    if (guild.Threads.has_value()) {
        for (auto &c : *guild.Threads) {
            m_joined_threads.insert(c.ID);
            c.GuildID = guild.ID;
            m_store.SetChannel(c.ID, c);
        }
    }

    for (auto &r : *guild.Roles)
        m_store.SetRole(guild.ID, r);

    for (auto &e : *guild.Emojis)
        m_store.SetEmoji(e.ID, e);

    m_store.EndTransaction();
}

void DiscordClient::HandleGatewayReady(const GatewayMessage &msg) {
    m_ready_received = true;

    if (m_dump_ready) {
        const auto name = "./payload_ready-" + Glib::DateTime::create_now_utc().format("%Y-%m-%d_%H-%M-%S") + ".json";
        auto *fp = std::fopen(name.c_str(), "wb");
        if (fp != nullptr) {
            const auto contents = msg.Data.dump(4);
            std::fwrite(contents.data(), contents.size(), 1, fp);
            std::fclose(fp);
        }
    }

    ReadyEventData data = msg.Data;
    for (auto &g : data.Guilds)
        ProcessNewGuild(g);

    m_store.BeginTransaction();

    for (const auto &dm : data.PrivateChannels) {
        m_guild_to_channels[Snowflake::Invalid].insert(dm.ID);
        m_store.SetChannel(dm.ID, dm);
        if (dm.Recipients.has_value())
            for (const auto &recipient : *dm.Recipients)
                m_store.SetUser(recipient.ID, recipient);
    }

    if (data.Users.has_value())
        for (const auto &user : *data.Users)
            m_store.SetUser(user.ID, user);

    if (data.MergedMembers.has_value()) {
        for (size_t i = 0; i < data.MergedMembers->size(); i++) {
            const auto guild_id = data.Guilds[i].ID;
            for (const auto &member : data.MergedMembers.value()[i]) {
                m_store.SetGuildMember(guild_id, *member.UserID, member);
            }
        }
    }

    if (data.Relationships.has_value())
        for (const auto &relationship : *data.Relationships)
            m_user_relationships[relationship.ID] = relationship.Type;

    if (data.GuildJoinRequests.has_value())
        for (const auto &request : *data.GuildJoinRequests)
            m_guild_join_requests[request.GuildID] = request;

    m_store.EndTransaction();

    m_session_id = data.SessionID;
    m_user_data = data.SelfUser;
    m_user_settings = data.Settings;
    m_user_guild_settings = data.GuildSettings;
    // TODO handle update

    HandleReadyReadState(data);
    HandleReadyGuildSettings(data);

    m_signal_gateway_ready.emit();
}

void DiscordClient::HandleGatewayMessageCreate(const GatewayMessage &msg) {
    Message data = msg.Data;
    StoreMessageData(data);
    if (data.GuildID.has_value())
        AddUserToGuild(data.Author.ID, *data.GuildID);
    m_last_message_id[data.ChannelID] = data.ID;
    if (data.Author.ID != GetUserData().ID)
        m_unread[data.ChannelID];
    if (data.DoesMention(GetUserData().ID)) {
        m_unread[data.ChannelID]++;
    }
    m_signal_message_create.emit(data);
}

void DiscordClient::HandleGatewayMessageDelete(const GatewayMessage &msg) {
    MessageDeleteData data = msg.Data;
    auto cur = m_store.GetMessage(data.ID);
    if (!cur.has_value())
        return;

    cur->SetDeleted();
    m_store.SetMessage(data.ID, *cur);
    m_signal_message_delete.emit(data.ID, data.ChannelID);
}

void DiscordClient::HandleGatewayMessageDeleteBulk(const GatewayMessage &msg) {
    MessageDeleteBulkData data = msg.Data;
    m_store.BeginTransaction();
    for (const auto &id : data.IDs) {
        auto cur = m_store.GetMessage(id);
        if (!cur.has_value())
            continue;

        cur->SetDeleted();
        m_store.SetMessage(id, *cur);
        m_signal_message_delete.emit(id, data.ChannelID);
    }
    m_store.EndTransaction();
}

void DiscordClient::HandleGatewayGuildMemberUpdate(const GatewayMessage &msg) {
    GuildMemberUpdateMessage data = msg.Data;
    auto cur = m_store.GetGuildMember(data.GuildID, data.User.ID);
    if (cur.has_value()) {
        cur->update_from_json(msg.Data);
        m_store.SetGuildMember(data.GuildID, data.User.ID, *cur);
    }
    m_signal_guild_member_update.emit(data.GuildID, data.User.ID);
}

void DiscordClient::HandleGatewayPresenceUpdate(const GatewayMessage &msg) {
    PresenceUpdateMessage data = msg.Data;
    const auto user_id = data.User.at("id").get<Snowflake>();

    auto cur = m_store.GetUser(user_id);
    if (cur.has_value()) {
        cur->update_from_json(data.User);
        m_store.SetUser(cur->ID, *cur);
    } else
        return;

    PresenceStatus e;
    if (data.StatusMessage == "online")
        e = PresenceStatus::Online;
    else if (data.StatusMessage == "offline")
        e = PresenceStatus::Offline;
    else if (data.StatusMessage == "idle")
        e = PresenceStatus::Idle;
    else if (data.StatusMessage == "dnd")
        e = PresenceStatus::DND;

    m_user_to_status[user_id] = e;

    m_signal_presence_update.emit(*cur, e);
}

void DiscordClient::HandleGatewayChannelDelete(const GatewayMessage &msg) {
    const auto id = msg.Data.at("id").get<Snowflake>();
    const auto channel = GetChannel(id);
    if (channel.has_value() && channel->GuildID.has_value()) {
        auto it = m_guild_to_channels.find(*channel->GuildID);
        if (it != m_guild_to_channels.end()) {
            it->second.erase(id);
        }
    }
    m_store.ClearChannel(id);
    m_signal_channel_delete.emit(id);
    m_signal_channel_accessibility_changed.emit(id, false);
}

void DiscordClient::HandleGatewayChannelUpdate(const GatewayMessage &msg) {
    const auto id = msg.Data.at("id").get<Snowflake>();
    auto cur = m_store.GetChannel(id);
    if (cur.has_value()) {
        const bool old_perms = HasChannelPermission(m_user_data.ID, id, Permission::VIEW_CHANNEL);

        cur->update_from_json(msg.Data);
        m_store.SetChannel(id, *cur);
        if (cur->PermissionOverwrites.has_value())
            for (const auto &p : *cur->PermissionOverwrites)
                m_store.SetPermissionOverwrite(id, p.ID, p);
        m_signal_channel_update.emit(id);

        const bool new_perms = HasChannelPermission(m_user_data.ID, id, Permission::VIEW_CHANNEL);
        if (old_perms && !new_perms)
            m_signal_channel_accessibility_changed.emit(id, false);
        else if (!old_perms && new_perms)
            m_signal_channel_accessibility_changed.emit(id, true);
    }
}

void DiscordClient::HandleGatewayChannelCreate(const GatewayMessage &msg) {
    ChannelData data = msg.Data;
    m_store.BeginTransaction();
    m_store.SetChannel(data.ID, data);
    m_guild_to_channels[*data.GuildID].insert(data.ID);
    if (data.PermissionOverwrites.has_value())
        for (const auto &p : *data.PermissionOverwrites)
            m_store.SetPermissionOverwrite(data.ID, p.ID, p);
    m_store.EndTransaction();
    m_signal_channel_create.emit(data);
}

void DiscordClient::HandleGatewayGuildUpdate(const GatewayMessage &msg) {
    Snowflake id = msg.Data.at("id");
    auto current = m_store.GetGuild(id);
    if (!current.has_value()) return;
    current->update_from_json(msg.Data);
    m_store.SetGuild(id, *current);
    m_signal_guild_update.emit(id);
}

void DiscordClient::HandleGatewayGuildRoleUpdate(const GatewayMessage &msg) {
    GuildRoleUpdateObject data = msg.Data;

    const auto channels = GetChannelsInGuild(data.GuildID);
    std::unordered_set<Snowflake> accessible;
    for (auto channel : channels) {
        if (HasChannelPermission(m_user_data.ID, channel, Permission::VIEW_CHANNEL))
            accessible.insert(channel);
    }

    m_store.SetRole(data.GuildID, data.Role);
    m_signal_role_update.emit(data.GuildID, data.Role.ID);

    for (auto channel : channels) {
        const bool old_perms = accessible.find(channel) != accessible.end();
        const bool new_perms = HasChannelPermission(m_user_data.ID, channel, Permission::VIEW_CHANNEL);
        if (old_perms && !new_perms) {
            m_signal_channel_accessibility_changed.emit(channel, false);
        } else if (!old_perms && new_perms) {
            m_signal_channel_accessibility_changed.emit(channel, true);
        }
    }
}

void DiscordClient::HandleGatewayGuildRoleCreate(const GatewayMessage &msg) {
    GuildRoleCreateObject data = msg.Data;
    m_store.SetRole(data.GuildID, data.Role);
    m_signal_role_create.emit(data.GuildID, data.Role.ID);
}

void DiscordClient::HandleGatewayGuildRoleDelete(const GatewayMessage &msg) {
    GuildRoleDeleteObject data = msg.Data;
    m_store.ClearRole(data.RoleID);
    m_signal_role_delete.emit(data.GuildID, data.RoleID);
}

void DiscordClient::HandleGatewayMessageReactionAdd(const GatewayMessage &msg) {
    MessageReactionAddObject data = msg.Data;

    if (data.Emoji.ID.IsValid() && !m_store.GetEmoji(data.Emoji.ID).has_value()) {
        m_store.SetEmoji(data.Emoji.ID, data.Emoji);
    }

    m_store.AddReaction(data, data.UserID == GetUserData().ID);
    if (data.Emoji.ID.IsValid()) {
        m_signal_reaction_add.emit(data.MessageID, std::to_string(data.Emoji.ID));
    } else {
        m_signal_reaction_add.emit(data.MessageID, data.Emoji.Name);
    }
}

void DiscordClient::HandleGatewayMessageReactionRemove(const GatewayMessage &msg) {
    MessageReactionRemoveObject data = msg.Data;

    if (data.Emoji.ID.IsValid() && !m_store.GetEmoji(data.Emoji.ID).has_value()) {
        m_store.SetEmoji(data.Emoji.ID, data.Emoji);
    }

    m_store.RemoveReaction(data, data.UserID == GetUserData().ID);
    if (data.Emoji.ID.IsValid()) {
        m_signal_reaction_remove.emit(data.MessageID, std::to_string(data.Emoji.ID));
    } else {
        m_signal_reaction_remove.emit(data.MessageID, data.Emoji.Name);
    }
}

// todo: update channel list item and member list
void DiscordClient::HandleGatewayChannelRecipientAdd(const GatewayMessage &msg) {
    ChannelRecipientAdd data = msg.Data;
    auto cur = m_store.GetChannel(data.ChannelID);
    if (!cur.has_value() || !cur->RecipientIDs.has_value()) return;
    if (std::find(cur->RecipientIDs->begin(), cur->RecipientIDs->end(), data.User.ID) == cur->RecipientIDs->end())
        cur->RecipientIDs->push_back(data.User.ID);
    m_store.SetUser(data.User.ID, data.User);
    m_store.SetChannel(cur->ID, *cur);
}

void DiscordClient::HandleGatewayChannelRecipientRemove(const GatewayMessage &msg) {
    ChannelRecipientRemove data = msg.Data;
    m_store.ClearRecipient(data.ChannelID, data.User.ID);
}

void DiscordClient::HandleGatewayTypingStart(const GatewayMessage &msg) {
    TypingStartObject data = msg.Data;
    Snowflake guild_id;
    if (data.GuildID.has_value()) {
        guild_id = *data.GuildID;
    } else {
        auto chan = m_store.GetChannel(data.ChannelID);
        if (chan.has_value() && chan->GuildID.has_value())
            guild_id = *chan->GuildID;
    }
    if (guild_id.IsValid() && data.Member.has_value()) {
        auto cur = m_store.GetGuildMember(guild_id, data.UserID);
        if (!cur.has_value()) {
            AddUserToGuild(data.UserID, guild_id);
            m_store.SetGuildMember(guild_id, data.UserID, *data.Member);
        }
        if (data.Member->User.has_value())
            m_store.SetUser(data.UserID, *data.Member->User);
    }
    m_signal_typing_start.emit(data.UserID, data.ChannelID);
}

void DiscordClient::HandleGatewayGuildBanRemove(const GatewayMessage &msg) {
    GuildBanRemoveObject data = msg.Data;
    m_store.SetUser(data.User.ID, data.User);
    m_store.ClearBan(data.GuildID, data.User.ID);
    m_signal_guild_ban_remove.emit(data.GuildID, data.User.ID);
}

void DiscordClient::HandleGatewayGuildBanAdd(const GatewayMessage &msg) {
    GuildBanAddObject data = msg.Data;
    BanData ban;
    ban.Reason = "";
    ban.User = data.User;
    m_store.SetUser(data.User.ID, data.User);
    m_store.SetBan(data.GuildID, data.User.ID, ban);
    m_signal_guild_ban_add.emit(data.GuildID, data.User.ID);
}

void DiscordClient::HandleGatewayInviteCreate(const GatewayMessage &msg) {
    InviteCreateObject data = msg.Data;
    InviteData invite;
    invite.Code = std::move(data.Code);
    invite.CreatedAt = std::move(data.CreatedAt);
    invite.Channel = *m_store.GetChannel(data.ChannelID);
    invite.Inviter = std::move(data.Inviter);
    invite.IsTemporary = data.IsTemporary;
    invite.MaxAge = data.MaxAge;
    invite.MaxUses = data.MaxUses;
    invite.TargetUser = std::move(data.TargetUser);
    invite.TargetUserType = data.TargetUserType;
    invite.Uses = data.Uses;
    if (data.GuildID.has_value())
        invite.Guild = m_store.GetGuild(*data.GuildID);
    m_signal_invite_create.emit(invite);
}

void DiscordClient::HandleGatewayInviteDelete(const GatewayMessage &msg) {
    InviteDeleteObject data = msg.Data;
    if (!data.GuildID.has_value()) {
        const auto chan = GetChannel(data.ChannelID);
        data.GuildID = chan->ID;
    }
    m_signal_invite_delete.emit(data);
}

void DiscordClient::HandleGatewayUserNoteUpdate(const GatewayMessage &msg) {
    UserNoteUpdateMessage data = msg.Data;
    m_signal_note_update.emit(data.ID, data.Note);
}

void DiscordClient::HandleGatewayGuildEmojisUpdate(const GatewayMessage &msg) {
    // like the real client, the emoji data sent in this message is ignored
    // we just use it as a signal to re-request all emojis
    GuildEmojisUpdateObject data = msg.Data;
    const auto cb = [this, id = data.GuildID](const std::vector<EmojiData> &emojis) {
        m_store.BeginTransaction();
        for (const auto &emoji : emojis)
            m_store.SetEmoji(emoji.ID, emoji);
        m_store.EndTransaction();
        m_signal_guild_emojis_update.emit(id, emojis);
    };
    FetchGuildEmojis(data.GuildID, cb);
}

void DiscordClient::HandleGatewayGuildJoinRequestCreate(const GatewayMessage &msg) {
    GuildJoinRequestCreateData data = msg.Data;
    m_guild_join_requests[data.GuildID] = data.Request;
    m_signal_guild_join_request_create.emit(data);
}

void DiscordClient::HandleGatewayGuildJoinRequestUpdate(const GatewayMessage &msg) {
    GuildJoinRequestUpdateData data = msg.Data;
    m_guild_join_requests[data.GuildID] = data.Request;
    m_signal_guild_join_request_update.emit(data);
}

void DiscordClient::HandleGatewayGuildJoinRequestDelete(const GatewayMessage &msg) {
    GuildJoinRequestDeleteData data = msg.Data;
    m_guild_join_requests.erase(data.GuildID);
    m_signal_guild_join_request_delete.emit(data);
}

void DiscordClient::HandleGatewayRelationshipRemove(const GatewayMessage &msg) {
    RelationshipRemoveData data = msg.Data;
    m_user_relationships.erase(data.ID);
    m_signal_relationship_remove.emit(data.ID, data.Type);
}

void DiscordClient::HandleGatewayRelationshipAdd(const GatewayMessage &msg) {
    RelationshipAddData data = msg.Data;
    m_store.SetUser(data.ID, data.User);
    m_user_relationships[data.ID] = data.Type;
    m_signal_relationship_add.emit(data);
}

// remarkably this doesnt actually mean a thread was created
// it can also mean you gained access to a thread. yay ...
// except sometimes it doesnt??? i dont know whats going on
void DiscordClient::HandleGatewayThreadCreate(const GatewayMessage &msg) {
    ThreadCreateData data = msg.Data;
    m_store.SetChannel(data.Channel.ID, data.Channel);
    m_signal_thread_create.emit(data.Channel);
    if (data.Channel.ThreadMember.has_value()) {
        m_signal_added_to_thread.emit(data.Channel.ID);
    }
}

void DiscordClient::HandleGatewayThreadDelete(const GatewayMessage &msg) {
    ThreadDeleteData data = msg.Data;
    m_store.ClearChannel(data.ID);
    m_signal_thread_delete.emit(data);
}

// this message is received when you load a channel as part of the lazy load request
// so the ui will only update thread when you load a channel in some guild
// which is rather annoying but oh well
void DiscordClient::HandleGatewayThreadListSync(const GatewayMessage &msg) {
    ThreadListSyncData data = msg.Data;
    for (const auto &thread : data.Threads)
        m_store.SetChannel(thread.ID, thread);
    m_signal_thread_list_sync.emit(data);
}

void DiscordClient::HandleGatewayThreadMembersUpdate(const GatewayMessage &msg) {
    ThreadMembersUpdateData data = msg.Data;
    if (data.AddedMembers.has_value() &&
        std::find_if(data.AddedMembers->begin(), data.AddedMembers->end(), [this](const auto &x) {
            return *x.UserID == m_user_data.ID; // safe to assume UserID is present here
        }) != data.AddedMembers->end()) {
        m_joined_threads.insert(data.ID);
        m_signal_added_to_thread.emit(data.ID);
    } else if (data.RemovedMemberIDs.has_value() &&
               std::find(data.RemovedMemberIDs->begin(), data.RemovedMemberIDs->end(), m_user_data.ID) != data.RemovedMemberIDs->end()) {
        m_joined_threads.erase(data.ID);
        m_signal_removed_from_thread.emit(data.ID);
    }
    m_signal_thread_members_update.emit(data);
}

void DiscordClient::HandleGatewayThreadMemberUpdate(const GatewayMessage &msg) {
    ThreadMemberUpdateData data = msg.Data;
    if (!data.Member.ThreadID.has_value()) return;

    m_joined_threads.insert(*data.Member.ThreadID);
    if (*data.Member.UserID == GetUserData().ID)
        m_signal_added_to_thread.emit(*data.Member.ThreadID);

    if (data.Member.IsMuted.has_value()) {
        const bool was_muted = IsChannelMuted(*data.Member.ThreadID);
        const bool now_muted = *data.Member.IsMuted;

        if (was_muted && !now_muted) {
            m_muted_channels.erase(*data.Member.ThreadID);
            m_signal_channel_unmuted.emit(*data.Member.ThreadID);
        } else if (!was_muted && now_muted) {
            m_muted_channels.insert(*data.Member.ThreadID);
            m_signal_channel_muted.emit(*data.Member.ThreadID);
        }
    }
}

void DiscordClient::HandleGatewayThreadUpdate(const GatewayMessage &msg) {
    ThreadUpdateData data = msg.Data;
    m_store.SetChannel(data.Thread.ID, data.Thread);
    m_signal_thread_update.emit(data);
}

void DiscordClient::HandleGatewayThreadMemberListUpdate(const GatewayMessage &msg) {
    ThreadMemberListUpdateData data = msg.Data;
    m_store.BeginTransaction();
    for (const auto &entry : data.Members) {
        m_thread_members[data.ThreadID].push_back(entry.UserID);
        if (entry.Member.User.has_value())
            m_store.SetUser(entry.Member.User->ID, *entry.Member.User);
        m_store.SetGuildMember(data.GuildID, entry.Member.User->ID, entry.Member);
    }
    m_store.EndTransaction();
    m_signal_thread_member_list_update.emit(data);
}

void DiscordClient::HandleGatewayMessageAck(const GatewayMessage &msg) {
    MessageAckData data = msg.Data;
    m_unread.erase(data.ChannelID);
    m_signal_message_ack.emit(data);
}

void DiscordClient::HandleGatewayUserGuildSettingsUpdate(const GatewayMessage &msg) {
    UserGuildSettingsUpdateData data = msg.Data;

    m_user_guild_settings.Entries[data.Settings.GuildID] = data.Settings;

    const bool for_dms = !data.Settings.GuildID.IsValid();

    const auto channels = for_dms ? GetPrivateChannels() : GetChannelsInGuild(data.Settings.GuildID);
    std::set<Snowflake> now_muted_channels;
    const auto now = Snowflake::FromNow();

    if (!for_dms) {
        const bool was_muted = IsGuildMuted(data.Settings.GuildID);
        bool now_muted = false;
        if (data.Settings.Muted) {
            if (data.Settings.MuteConfig.EndTime.has_value()) {
                const auto end = Snowflake::FromISO8601(*data.Settings.MuteConfig.EndTime);
                if (end.IsValid() && end > now)
                    now_muted = true;
            } else {
                now_muted = true;
            }
        }
        if (was_muted && !now_muted) {
            m_muted_guilds.erase(data.Settings.GuildID);
            m_signal_guild_unmuted.emit(data.Settings.GuildID);
        } else if (!was_muted && now_muted) {
            m_muted_guilds.insert(data.Settings.GuildID);
            m_signal_guild_muted.emit(data.Settings.GuildID);
        }
    }

    for (const auto &override : data.Settings.ChannelOverrides) {
        if (override.Muted) {
            if (override.MuteConfig.EndTime.has_value()) {
                const auto end = Snowflake::FromISO8601(*override.MuteConfig.EndTime);
                if (end.IsValid() && end > now)
                    now_muted_channels.insert(override.ChannelID);
            } else {
                now_muted_channels.insert(override.ChannelID);
            }
        }
    }
    for (const auto &channel_id : channels) {
        const bool was_muted = IsChannelMuted(channel_id);
        const bool now_muted = now_muted_channels.find(channel_id) != now_muted_channels.end();
        if (now_muted) {
            m_muted_channels.insert(channel_id);
            if (!was_muted) {
                if (const auto chan = GetChannel(channel_id); chan.has_value() && chan->IsCategory())
                    for (const auto child_id : chan->GetChildIDs())
                        m_channel_muted_parent.insert(child_id);

                m_signal_channel_muted.emit(channel_id);
            }
        } else {
            m_muted_channels.erase(channel_id);
            if (was_muted) {
                if (const auto chan = GetChannel(channel_id); chan.has_value() && chan->IsCategory())
                    for (const auto child_id : chan->GetChildIDs())
                        m_channel_muted_parent.erase(child_id);

                m_signal_channel_unmuted.emit(channel_id);
            }
        }
    }
}

void DiscordClient::HandleGatewayGuildMembersChunk(const GatewayMessage &msg) {
    GuildMembersChunkData data = msg.Data;
    m_store.BeginTransaction();
    for (const auto &member : data.Members)
        m_store.SetGuildMember(data.GuildID, member.User->ID, member);
    m_store.EndTransaction();
}

#ifdef WITH_VOICE

/*
 * When you connect to a voice channel:
 * C->S: VOICE_STATE_UPDATE
 * S->C: VOICE_STATE_UPDATE
 * S->C: VOICE_SERVER_UPDATE
 *
 * A new websocket is opened to the ws specified by VOICE_SERVER_UPDATE then:
 * S->C: HELLO
 * C->S: IDENTIFY
 * S->C: READY
 * C->U: discover
 * U->C: discover result
 * C->S: SELECT_PROTOCOL
 * S->C: SESSION_DESCRIPTION
 * Done!!!
 *
 * When you get disconnected (no particular order):
 * S->C: 4014 Disconnected (Server to voice gateway)
 * S->C: VOICE_STATE_UPDATE (Server to main gateway)
 *
 * When you get moved:
 * S->C: VOICE_STATE_UPDATE
 * S->C: VOICE_SERVER_UPDATE (usually)
 * S->C: 4014 Disconnected (Server to voice gateway)
 *
 * Key thing: 4014 Disconnected can come before or after or in between main gateway messages
 *
 * Region change:
 * Same thing but close code 4000
 *
 */

void DiscordClient::HandleGatewayVoiceStateUpdate(const GatewayMessage &msg) {
    spdlog::get("discord")->trace("VOICE_STATE_UPDATE");

    CheckVoiceState(msg.Data);
}

void DiscordClient::HandleGatewayVoiceServerUpdate(const GatewayMessage &msg) {
    spdlog::get("discord")->trace("VOICE_SERVER_UPDATE");

    VoiceServerUpdateData data = msg.Data;
    spdlog::get("discord")->debug("Voice server endpoint: {}", data.Endpoint);
    spdlog::get("discord")->debug("Voice token: {}", data.Token);
    m_voice.SetEndpoint(data.Endpoint);
    m_voice.SetToken(data.Token);
    if (data.GuildID.has_value()) {
        m_voice.SetServerID(*data.GuildID);
    } else if (data.ChannelID.has_value()) {
        m_voice.SetServerID(*data.ChannelID);
    } else {
        spdlog::get("discord")->error("No guild or channel ID in voice server?");
    }
    m_voice.SetUserID(m_user_data.ID);
    m_voice.Start();
}

void DiscordClient::HandleGatewayCallCreate(const GatewayMessage &msg) {
    CallCreateData data = msg.Data;

    spdlog::get("discord")->debug("CALL_CREATE: {}", data.ChannelID);

    for (const auto &state : data.VoiceStates) {
        CheckVoiceState(state);
    }
}

void DiscordClient::CheckVoiceState(const VoiceState &data) {
    if (data.UserID == m_user_data.ID) {
        spdlog::get("discord")->debug("Voice session ID: {}", data.SessionID);
        m_voice.SetSessionID(data.SessionID);

        // channel_id = null means disconnect. stop cuz out of order maybe
        if (!data.ChannelID.has_value() && (m_voice.IsConnected() || m_voice.IsConnecting())) {
            m_voice.Stop();
        } else if (data.ChannelID.has_value()) {
            m_voice_channel_id = *data.ChannelID;
            m_signal_voice_channel_changed.emit(m_voice_channel_id);
        }
    } else {
        if (data.GuildID.has_value() && data.Member.has_value()) {
            if (data.Member->User.has_value()) {
                m_store.SetUser(data.UserID, *data.Member->User);
            }
            m_store.SetGuildMember(*data.GuildID, data.UserID, *data.Member);
        }
    }

    if (data.ChannelID.has_value()) {
        const auto old_state = GetVoiceState(data.UserID);
        SetVoiceState(data.UserID, data);
        if (old_state.has_value() && old_state->first != *data.ChannelID) {
            m_signal_voice_user_disconnect.emit(data.UserID, old_state->first);
            m_signal_voice_user_connect.emit(data.UserID, *data.ChannelID);
        } else if (!old_state.has_value()) {
            m_signal_voice_user_connect.emit(data.UserID, *data.ChannelID);
        }
    } else {
        const auto old_state = GetVoiceState(data.UserID);
        ClearVoiceState(data.UserID);
        if (old_state.has_value()) {
            m_signal_voice_user_disconnect.emit(data.UserID, old_state->first);
        }
    }
}
#endif

void DiscordClient::HandleGatewayReadySupplemental(const GatewayMessage &msg) {
    ReadySupplementalData data = msg.Data;

    const auto handle_presence = [this](const MergedPresence &p) {
        const auto user = GetUser(p.UserID);
        if (!user.has_value()) return; // should be sent in READY's `users`
        const auto s = p.Presence.Status;
        if (s == "online")
            m_user_to_status[p.UserID] = PresenceStatus::Online;
        else if (s == "offline")
            m_user_to_status[p.UserID] = PresenceStatus::Offline;
        else if (s == "idle")
            m_user_to_status[p.UserID] = PresenceStatus::Idle;
        else if (s == "dnd")
            m_user_to_status[p.UserID] = PresenceStatus::DND;
        m_signal_presence_update.emit(*user, m_user_to_status.at(p.UserID));
    };

    for (const auto &p : data.MergedPresences.Friends) {
        handle_presence(p);
    }
    for (const auto &g : data.MergedPresences.Guilds) {
        for (const auto &p : g) {
            handle_presence(p);
        }
    }
#ifdef WITH_VOICE
    for (const auto &g : data.Guilds) {
        for (const auto &s : g.VoiceStates) {
            if (s.ChannelID.has_value()) {
                SetVoiceState(s.UserID, s);
            }
        }
    }
#endif

    m_signal_gateway_ready_supplemental.emit();
}

void DiscordClient::HandleGatewayReconnect(const GatewayMessage &msg) {
    printf("received reconnect\n");
    inflateEnd(&m_zstream);
    m_compressed_buf.clear();

    m_heartbeat_waiter.kill();
    if (m_heartbeat_thread.joinable()) m_heartbeat_thread.join();

    m_reconnecting = true;
    m_wants_resume = true;
    m_heartbeat_acked = true;

    m_websocket.Stop(1012); // 1000 (kNormalClosureCode) and 1001 will invalidate the session id

    std::memset(&m_zstream, 0, sizeof(m_zstream));
    inflateInit2(&m_zstream, MAX_WBITS + 32);

    m_websocket.StartConnection(GetGatewayURL());
}

void DiscordClient::HandleGatewayInvalidSession(const GatewayMessage &msg) {
    printf("invalid session! re-identifying\n");

    inflateEnd(&m_zstream);
    m_compressed_buf.clear();

    std::memset(&m_zstream, 0, sizeof(m_zstream));
    inflateInit2(&m_zstream, MAX_WBITS + 32);

    m_heartbeat_acked = true;
    m_wants_resume = false;
    m_reconnecting = true;

    m_heartbeat_waiter.kill();
    if (m_heartbeat_thread.joinable()) m_heartbeat_thread.join();

    m_websocket.Stop(1000);

    if (m_client_started)
        Glib::signal_timeout().connect_once([this] { if (m_client_started) m_websocket.StartConnection(GetGatewayURL()); }, 1000);
}

bool IsCompleteMessageObject(const nlohmann::json &j) {
    const auto required = { "id", "channel_id", "author", "content", "timestamp", "edited_timestamp", "tts", "mention_everyone", "mention_roles", "attachments", "embeds", "pinned", "type" };
    for (const auto &str : required) {
        if (!j.contains(str)) return false;
    }
    return true;
}

void DiscordClient::HandleGatewayMessageUpdate(const GatewayMessage &msg) {
    Snowflake id = msg.Data.at("id");

    auto current = m_store.GetMessage(id);
    if (!current.has_value()) {
        // im not sure how the client determines if a MESSAGE_UPDATE is suitable to be stored as a full message but i guess its something like this
        if (IsCompleteMessageObject(msg.Data)) {
            current = msg.Data;
            m_store.SetMessage(id, *current);
            // this doesnt mean a message is newly pinned when called here
            // it just means theres an (old) message that the client is now aware of that is also pinned
            m_signal_message_pinned.emit(*current);
        } else
            return;
    } else {
        const bool old_pinned = current->IsPinned;

        current->from_json_edited(msg.Data);
        m_store.SetMessage(id, *current);

        if (old_pinned && !current->IsPinned)
            m_signal_message_unpinned.emit(*current);
        else if (!old_pinned && current->IsPinned)
            m_signal_message_pinned.emit(*current);
    }

    m_signal_message_update.emit(id, current->ChannelID);
}

void DiscordClient::HandleGatewayGuildMemberListUpdate(const GatewayMessage &msg) {
    GuildMemberListUpdateMessage data = msg.Data;

    m_store.BeginTransaction();

    bool has_sync = false;
    for (const auto &op : data.Ops) {
        if (op.Op == "SYNC") {
            has_sync = true;
            for (const auto &item : *op.Items) {
                if (item->Type == "member") {
                    auto member = dynamic_cast<const GuildMemberListUpdateMessage::MemberItem *>(item.get());
                    m_store.SetUser(member->User.ID, member->User);
                    AddUserToGuild(member->User.ID, data.GuildID);
                    m_store.SetGuildMember(data.GuildID, member->User.ID, member->GetAsMemberData());
                    if (member->Presence.has_value()) {
                        const auto &s = member->Presence->Status;
                        if (s == "online")
                            m_user_to_status[member->User.ID] = PresenceStatus::Online;
                        else if (s == "offline")
                            m_user_to_status[member->User.ID] = PresenceStatus::Offline;
                        else if (s == "idle")
                            m_user_to_status[member->User.ID] = PresenceStatus::Idle;
                        else if (s == "dnd")
                            m_user_to_status[member->User.ID] = PresenceStatus::DND;
                    }
                }
            }
        } else if (op.Op == "UPDATE") {
            if (op.OpItem.has_value() && op.OpItem.value()->Type == "member") {
                const auto &m = dynamic_cast<const GuildMemberListUpdateMessage::MemberItem *>(op.OpItem.value().get())->GetAsMemberData();
                m_store.SetGuildMember(data.GuildID, m.User->ID, m);
                m_signal_guild_member_update.emit(data.GuildID, m.User->ID); // cheeky
            }
        }
    }

    m_store.EndTransaction();

    // todo: manage this event a little better
    if (has_sync)
        m_signal_guild_member_list_update.emit(data.GuildID);
}

void DiscordClient::HandleGatewayGuildCreate(const GatewayMessage &msg) {
    GuildData data = msg.Data;

    // TODO: figure out why this is even happening... maybe?
    // ignore guild if already stored
    if (m_store.GetGuild(data.ID).has_value()) {
        return;
    }

    ProcessNewGuild(data);

    m_signal_guild_create.emit(data);
}

void DiscordClient::HandleGatewayGuildDelete(const GatewayMessage &msg) {
    Snowflake id = msg.Data.at("id");
    bool unavailable = msg.Data.contains("unavailable") && msg.Data.at("unavailable").get<bool>();

    if (unavailable)
        printf("guild %" PRIu64 " became unavailable\n", static_cast<uint64_t>(id));

    const auto guild = m_store.GetGuild(id);
    if (!guild.has_value()) {
        m_store.ClearGuild(id);
        m_signal_guild_delete.emit(id);
        return;
    }

    m_store.ClearGuild(id);
    if (guild->Channels.has_value()) {
        for (const auto &c : *guild->Channels) {
            m_store.ClearChannel(c.ID);
            m_signal_channel_accessibility_changed.emit(c.ID, false);
        }
    }

    m_signal_guild_delete.emit(id);
}

void DiscordClient::AddUserToGuild(Snowflake user_id, Snowflake guild_id) {
    m_guild_to_users[guild_id].insert(user_id);
}

std::set<Snowflake> DiscordClient::GetPrivateChannels() const {
    if (const auto iter = m_guild_to_channels.find(Snowflake::Invalid); iter != m_guild_to_channels.end())
        return iter->second;
    return {};
}

const UserSettings &DiscordClient::GetUserSettings() const {
    return m_user_settings;
}

EPremiumType DiscordClient::GetSelfPremiumType() const {
    const auto &data = GetUserData();
    if (data.PremiumType.has_value())
        return *data.PremiumType;
    return EPremiumType::None;
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
    IdentifyMessage msg;
    msg.Token = m_token;
    msg.Capabilities = 4605; // bit 12 is necessary for CALL_CREATE... apparently? need to get this in sync with official client
    msg.Properties.OS = "Windows";
    msg.Properties.Browser = "Chrome";
    msg.Properties.Device = "";
    msg.Properties.SystemLocale = "en-US";
    msg.Properties.BrowserUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36";
    msg.Properties.BrowserVersion = "67.0.3396.87";
    msg.Properties.OSVersion = "10";
    msg.Properties.Referrer = "";
    msg.Properties.ReferringDomain = "";
    msg.Properties.ReferrerCurrent = "";
    msg.Properties.ReferringDomainCurrent = "";
    msg.Properties.ReleaseChannel = "stable";
    msg.Properties.ClientBuildNumber = m_build_number;
    msg.Properties.ClientEventSource = "";
    msg.Presence.Status = "online";
    msg.Presence.Since = 0;
    msg.Presence.IsAFK = false;
    msg.DoesSupportCompression = false;
    SetSuperPropertiesFromIdentity(msg);
    const bool b = m_websocket.GetPrintMessages();
    m_websocket.SetPrintMessages(false);
    m_websocket.Send(msg);
    m_websocket.SetPrintMessages(b);
}

void DiscordClient::SendResume() {
    ResumeMessage msg;
    msg.Sequence = m_last_sequence;
    msg.SessionID = m_session_id;
    msg.Token = m_token;
    m_websocket.Send(msg);
}

void DiscordClient::SetHeaders() {
    m_http.SetPersistentHeader("Sec-Fetch-Dest", "empty");
    m_http.SetPersistentHeader("Sec-Fetch-Mode", "cors");
    m_http.SetPersistentHeader("Sec-Fetch-Site", "same-origin");
    m_http.SetPersistentHeader("X-Debug-Options", "bugReporterEnabled");
    m_http.SetPersistentHeader("Accept-Language", "en-US,en;q=0.9");

    SetReferringChannel(Snowflake::Invalid);
}

void DiscordClient::SetSuperPropertiesFromIdentity(const IdentifyMessage &identity) {
    nlohmann::ordered_json j;
    j["os"] = identity.Properties.OS;
    j["browser"] = identity.Properties.Browser;
    j["device"] = identity.Properties.Device;
    j["system_locale"] = identity.Properties.SystemLocale;
    j["browser_user_agent"] = identity.Properties.BrowserUserAgent;
    j["browser_version"] = identity.Properties.BrowserVersion;
    j["os_version"] = identity.Properties.OSVersion;
    j["referrer"] = identity.Properties.Referrer;
    j["referring_domain"] = identity.Properties.ReferringDomain;
    j["referrer_current"] = identity.Properties.ReferrerCurrent;
    j["referring_domain_current"] = identity.Properties.ReferringDomainCurrent;
    j["release_channel"] = identity.Properties.ReleaseChannel;
    j["client_build_number"] = identity.Properties.ClientBuildNumber;
    j["client_event_source"] = nullptr; // probably will never be non-null ("") anyways
    m_http.SetPersistentHeader("X-Super-Properties", Glib::Base64::encode(j.dump()));
    m_http.SetPersistentHeader("X-Discord-Locale", identity.Properties.SystemLocale);
}

void DiscordClient::HandleSocketOpen() {
}

void DiscordClient::HandleSocketClose(const ix::WebSocketCloseInfo &info) {
    auto close_code = static_cast<GatewayCloseCode>(info.code);
    auto cb = [this, close_code]() {
        m_heartbeat_waiter.kill();
        if (m_heartbeat_thread.joinable()) m_heartbeat_thread.join();
        m_client_connected = false;

        if (m_client_started && !m_reconnecting && close_code == GatewayCloseCode::Abnormal) {
            Glib::signal_timeout().connect_once([this] { if (m_client_started) HandleGatewayReconnect(GatewayMessage()); }, 1000);
            m_reconnecting = true;
        }

        m_signal_disconnected.emit(m_reconnecting, close_code);
    };
    m_generic_mutex.lock();
    m_generic_queue.push(cb);
    m_generic_dispatch.emit();
    m_generic_mutex.unlock();
}

bool DiscordClient::CheckCode(const http::response_type &r) {
    if (r.status_code >= 300 || r.error) {
        fprintf(stderr, "api request to %s failed with status code %d: %s\n", r.url.c_str(), r.status_code, r.error_string.c_str());
        return false;
    }

    return true;
}

bool DiscordClient::CheckCode(const http::response_type &r, int expected) {
    if (!CheckCode(r)) return false;
    if (r.status_code != expected) {
        fprintf(stderr, "api request to %s returned %d, expected %d\n", r.url.c_str(), r.status_code, expected);
        return false;
    }
    return true;
}

void DiscordClient::StoreMessageData(Message &msg) {
    const auto chan = m_store.GetChannel(msg.ChannelID);
    if (chan.has_value() && chan->GuildID.has_value())
        msg.GuildID = *chan->GuildID;

    m_store.BeginTransaction();

    m_store.SetMessage(msg.ID, msg);
    m_store.SetUser(msg.Author.ID, msg.Author);
    if (msg.Reactions.has_value())
        for (const auto &r : *msg.Reactions) {
            if (!r.Emoji.ID.IsValid()) continue;
            const auto cur = m_store.GetEmoji(r.Emoji.ID);
            if (!cur.has_value()) {
                m_store.SetEmoji(r.Emoji.ID, r.Emoji);
            }
        }

    for (const auto &user : msg.Mentions) {
        m_store.SetUser(user.ID, user);
    }

    if (msg.Member.has_value()) {
        m_store.SetGuildMember(*msg.GuildID, msg.Author.ID, *msg.Member);
    }

    if (msg.Interaction.has_value()) {
        m_store.SetUser(msg.Interaction->User.ID, msg.Interaction->User);
        if (msg.Interaction->Member.has_value()) {
            m_store.SetGuildMember(*msg.GuildID, msg.Interaction->User.ID, *msg.Interaction->Member);
        }
    }

    if (msg.IsWebhook()) {
        m_store.SetWebhookMessage(msg);
    }

    m_store.EndTransaction();

    if (msg.ReferencedMessage.has_value() && msg.MessageReference.has_value() && msg.MessageReference->ChannelID.has_value()) {
        if (msg.ReferencedMessage.value() != nullptr) {
            StoreMessageData(**msg.ReferencedMessage);
        }
    }
}

// some notes for myself
// a read channel is determined by checking if the channel object's last message id is equal to the read state's last message id
// channels without entries are also unread
// here the absence of an entry in m_unread indicates a read channel and the value is only the mention count since the message doesnt matter
// no entry.id cannot be a guild even though sometimes it looks like it
void DiscordClient::HandleReadyReadState(const ReadyEventData &data) {
    for (const auto &guild : data.Guilds) {
        if (guild.Channels.has_value()) {
            for (const auto &channel : *guild.Channels) {
                if (channel.LastMessageID.has_value()) {
                    m_last_message_id[channel.ID] = *channel.LastMessageID;
                }
            }
        }
        if (guild.Threads.has_value()) {
            for (const auto &thread : *guild.Threads) {
                if (thread.LastMessageID.has_value()) {
                    m_last_message_id[thread.ID] = *thread.LastMessageID;
                }
            }
        }
    }
    for (const auto &channel : data.PrivateChannels) {
        if (channel.LastMessageID.has_value()) {
            m_last_message_id[channel.ID] = *channel.LastMessageID;
        }
    }
    for (const auto &entry : data.ReadState.Entries) {
        const auto it = m_last_message_id.find(entry.ID);
        if (it == m_last_message_id.end()) continue;
        if (it->second > entry.LastMessageID) {
            if (HasChannelPermission(GetUserData().ID, entry.ID, Permission::VIEW_CHANNEL))
                m_unread[entry.ID] = entry.MentionCount;
        }
    }

    // channels that arent in the read state are considered unread
    for (const auto &guild : data.Guilds) {
        if (!guild.JoinedAt.has_value()) continue; // doubt this can happen but whatever
        const auto joined_at = Snowflake::FromISO8601(*guild.JoinedAt);
        for (const auto &channel : *guild.Channels) {
            if (channel.LastMessageID.has_value()) {
                // unread messages from before you joined dont count as unread
                if (*channel.LastMessageID < joined_at) continue;
                if (std::find_if(data.ReadState.Entries.begin(), data.ReadState.Entries.end(), [id = channel.ID](const ReadStateEntry &e) {
                        return e.ID == id;
                    }) == data.ReadState.Entries.end()) {
                    // cant be unread if u cant even see the channel
                    // better to check here since HasChannelPermission hits the store
                    if (HasChannelPermission(GetUserData().ID, channel.ID, Permission::VIEW_CHANNEL))
                        m_unread[channel.ID] = 0;
                }
            }
        }
    }
}

void DiscordClient::HandleReadyGuildSettings(const ReadyEventData &data) {
    // i dont like this implementation for muted categories but its rather simple and doesnt use a horriiible amount of ram

    std::unordered_map<Snowflake, std::vector<Snowflake>> category_children;
    for (const auto &guild : data.Guilds) {
        for (const auto &channel : *guild.Channels)
            if (channel.ParentID.has_value() && !channel.IsThread())
                category_children[*channel.ParentID].push_back(channel.ID);
        for (const auto &thread : *guild.Threads)
            if (thread.ThreadMember.has_value() && thread.ThreadMember->IsMuted.has_value() && *thread.ThreadMember->IsMuted)
                m_muted_channels.insert(thread.ID);
    }

    const auto now = Snowflake::FromNow();
    for (const auto &[guild_id, entry] : data.GuildSettings.Entries) {
        // even if muted is true a guild/channel can be unmuted if the current time passes mute_config.end_time
        if (entry.Muted) {
            if (entry.MuteConfig.EndTime.has_value()) {
                const auto end = Snowflake::FromISO8601(*entry.MuteConfig.EndTime);
                if (end.IsValid() && end > now)
                    m_muted_guilds.insert(entry.GuildID);
            } else {
                m_muted_guilds.insert(entry.GuildID);
            }
        }
        for (const auto &override : entry.ChannelOverrides) {
            if (override.Muted) {
                if (const auto iter = category_children.find(override.ChannelID); iter != category_children.end())
                    for (const auto child : iter->second)
                        m_channel_muted_parent.insert(child);

                if (override.MuteConfig.EndTime.has_value()) {
                    const auto end = Snowflake::FromISO8601(*override.MuteConfig.EndTime);
                    if (end.IsValid() && end > now)
                        m_muted_channels.insert(override.ChannelID);
                } else {
                    m_muted_channels.insert(override.ChannelID);
                }
            }
        }
    }
}

#ifdef WITH_VOICE
void DiscordClient::SendVoiceStateUpdate() {
    VoiceStateUpdateMessage msg;
    msg.ChannelID = m_voice_channel_id;
    const auto channel = GetChannel(m_voice_channel_id);
    if (channel.has_value() && channel->GuildID.has_value()) {
        msg.GuildID = *channel->GuildID;
    }

    msg.SelfMute = m_mute_requested;
    msg.SelfDeaf = m_deaf_requested;
    msg.SelfVideo = false;

    m_websocket.Send(msg);
}

void DiscordClient::SetVoiceState(Snowflake user_id, const VoiceState &state) {
    if (!state.ChannelID.has_value()) {
        spdlog::get("discord")->error("SetVoiceState called with missing channel ID");
        return;
    }

    auto flags = VoiceStateFlags::Clear;
    if (state.IsSelfMuted) flags |= VoiceStateFlags::SelfMute;
    if (state.IsSelfDeafened) flags |= VoiceStateFlags::SelfDeaf;
    if (state.IsMuted) flags |= VoiceStateFlags::Mute;
    if (state.IsDeafened) flags |= VoiceStateFlags::Deaf;
    if (state.IsSelfStream) flags |= VoiceStateFlags::SelfStream;
    if (state.IsSelfVideo) flags |= VoiceStateFlags::SelfVideo;

    m_voice_states[user_id] = std::make_pair(*state.ChannelID, flags);
    m_voice_state_channel_users[*state.ChannelID].insert(user_id);

    m_signal_voice_state_set.emit(user_id, *state.ChannelID, flags);
}

void DiscordClient::ClearVoiceState(Snowflake user_id) {
    spdlog::get("discord")->debug("ClearVoiceState: {}", user_id);
    if (const auto it = m_voice_states.find(user_id); it != m_voice_states.end()) {
        m_voice_state_channel_users[it->second.first].erase(user_id);
        // invalidated
        m_voice_states.erase(user_id);
    }
}

void DiscordClient::OnVoiceConnected() {
    m_signal_voice_connected.emit();
}

void DiscordClient::OnVoiceDisconnected() {
    m_signal_voice_disconnected.emit();
}
#endif

void DiscordClient::LoadEventMap() {
    m_event_map["READY"] = GatewayEvent::READY;
    m_event_map["MESSAGE_CREATE"] = GatewayEvent::MESSAGE_CREATE;
    m_event_map["MESSAGE_DELETE"] = GatewayEvent::MESSAGE_DELETE;
    m_event_map["MESSAGE_UPDATE"] = GatewayEvent::MESSAGE_UPDATE;
    m_event_map["GUILD_MEMBER_LIST_UPDATE"] = GatewayEvent::GUILD_MEMBER_LIST_UPDATE;
    m_event_map["GUILD_CREATE"] = GatewayEvent::GUILD_CREATE;
    m_event_map["GUILD_DELETE"] = GatewayEvent::GUILD_DELETE;
    m_event_map["MESSAGE_DELETE_BULK"] = GatewayEvent::MESSAGE_DELETE_BULK;
    m_event_map["GUILD_MEMBER_UPDATE"] = GatewayEvent::GUILD_MEMBER_UPDATE;
    m_event_map["PRESENCE_UPDATE"] = GatewayEvent::PRESENCE_UPDATE;
    m_event_map["CHANNEL_DELETE"] = GatewayEvent::CHANNEL_DELETE;
    m_event_map["CHANNEL_UPDATE"] = GatewayEvent::CHANNEL_UPDATE;
    m_event_map["CHANNEL_CREATE"] = GatewayEvent::CHANNEL_CREATE;
    m_event_map["GUILD_UPDATE"] = GatewayEvent::GUILD_UPDATE;
    m_event_map["GUILD_ROLE_UPDATE"] = GatewayEvent::GUILD_ROLE_UPDATE;
    m_event_map["GUILD_ROLE_CREATE"] = GatewayEvent::GUILD_ROLE_CREATE;
    m_event_map["GUILD_ROLE_DELETE"] = GatewayEvent::GUILD_ROLE_DELETE;
    m_event_map["MESSAGE_REACTION_ADD"] = GatewayEvent::MESSAGE_REACTION_ADD;
    m_event_map["MESSAGE_REACTION_REMOVE"] = GatewayEvent::MESSAGE_REACTION_REMOVE;
    m_event_map["CHANNEL_RECIPIENT_ADD"] = GatewayEvent::CHANNEL_RECIPIENT_ADD;
    m_event_map["CHANNEL_RECIPIENT_REMOVE"] = GatewayEvent::CHANNEL_RECIPIENT_REMOVE;
    m_event_map["TYPING_START"] = GatewayEvent::TYPING_START;
    m_event_map["GUILD_BAN_REMOVE"] = GatewayEvent::GUILD_BAN_REMOVE;
    m_event_map["GUILD_BAN_ADD"] = GatewayEvent::GUILD_BAN_ADD;
    m_event_map["INVITE_CREATE"] = GatewayEvent::INVITE_CREATE;
    m_event_map["INVITE_DELETE"] = GatewayEvent::INVITE_DELETE;
    m_event_map["USER_NOTE_UPDATE"] = GatewayEvent::USER_NOTE_UPDATE;
    m_event_map["READY_SUPPLEMENTAL"] = GatewayEvent::READY_SUPPLEMENTAL;
    m_event_map["GUILD_EMOJIS_UPDATE"] = GatewayEvent::GUILD_EMOJIS_UPDATE;
    m_event_map["GUILD_JOIN_REQUEST_CREATE"] = GatewayEvent::GUILD_JOIN_REQUEST_CREATE;
    m_event_map["GUILD_JOIN_REQUEST_UPDATE"] = GatewayEvent::GUILD_JOIN_REQUEST_UPDATE;
    m_event_map["GUILD_JOIN_REQUEST_DELETE"] = GatewayEvent::GUILD_JOIN_REQUEST_DELETE;
    m_event_map["RELATIONSHIP_REMOVE"] = GatewayEvent::RELATIONSHIP_REMOVE;
    m_event_map["RELATIONSHIP_ADD"] = GatewayEvent::RELATIONSHIP_ADD;
    m_event_map["THREAD_CREATE"] = GatewayEvent::THREAD_CREATE;
    m_event_map["THREAD_DELETE"] = GatewayEvent::THREAD_DELETE;
    m_event_map["THREAD_LIST_SYNC"] = GatewayEvent::THREAD_LIST_SYNC;
    m_event_map["THREAD_MEMBERS_UPDATE"] = GatewayEvent::THREAD_MEMBERS_UPDATE;
    m_event_map["THREAD_MEMBER_UPDATE"] = GatewayEvent::THREAD_MEMBER_UPDATE;
    m_event_map["THREAD_UPDATE"] = GatewayEvent::THREAD_UPDATE;
    m_event_map["THREAD_MEMBER_LIST_UPDATE"] = GatewayEvent::THREAD_MEMBER_LIST_UPDATE;
    m_event_map["MESSAGE_ACK"] = GatewayEvent::MESSAGE_ACK;
    m_event_map["USER_GUILD_SETTINGS_UPDATE"] = GatewayEvent::USER_GUILD_SETTINGS_UPDATE;
    m_event_map["GUILD_MEMBERS_CHUNK"] = GatewayEvent::GUILD_MEMBERS_CHUNK;
    m_event_map["VOICE_STATE_UPDATE"] = GatewayEvent::VOICE_STATE_UPDATE;
    m_event_map["VOICE_SERVER_UPDATE"] = GatewayEvent::VOICE_SERVER_UPDATE;
    m_event_map["CALL_CREATE"] = GatewayEvent::CALL_CREATE;
}

DiscordClient::type_signal_gateway_ready DiscordClient::signal_gateway_ready() {
    return m_signal_gateway_ready;
}

DiscordClient::type_signal_gateway_ready_supplemental DiscordClient::signal_gateway_ready_supplemental() {
    return m_signal_gateway_ready_supplemental;
}

DiscordClient::type_signal_message_create DiscordClient::signal_message_create() {
    return m_signal_message_create;
}

DiscordClient::type_signal_message_delete DiscordClient::signal_message_delete() {
    return m_signal_message_delete;
}

DiscordClient::type_signal_message_update DiscordClient::signal_message_update() {
    return m_signal_message_update;
}

DiscordClient::type_signal_guild_member_list_update DiscordClient::signal_guild_member_list_update() {
    return m_signal_guild_member_list_update;
}

DiscordClient::type_signal_guild_create DiscordClient::signal_guild_create() {
    return m_signal_guild_create;
}

DiscordClient::type_signal_guild_delete DiscordClient::signal_guild_delete() {
    return m_signal_guild_delete;
}

DiscordClient::type_signal_channel_delete DiscordClient::signal_channel_delete() {
    return m_signal_channel_delete;
}

DiscordClient::type_signal_channel_update DiscordClient::signal_channel_update() {
    return m_signal_channel_update;
}

DiscordClient::type_signal_channel_create DiscordClient::signal_channel_create() {
    return m_signal_channel_create;
}

DiscordClient::type_signal_guild_update DiscordClient::signal_guild_update() {
    return m_signal_guild_update;
}

DiscordClient::type_signal_disconnected DiscordClient::signal_disconnected() {
    return m_signal_disconnected;
}

DiscordClient::type_signal_connected DiscordClient::signal_connected() {
    return m_signal_connected;
}

DiscordClient::type_signal_message_progress DiscordClient::signal_message_progress() {
    return m_signal_message_progress;
}

DiscordClient::type_signal_role_update DiscordClient::signal_role_update() {
    return m_signal_role_update;
}

DiscordClient::type_signal_role_create DiscordClient::signal_role_create() {
    return m_signal_role_create;
}

DiscordClient::type_signal_role_delete DiscordClient::signal_role_delete() {
    return m_signal_role_delete;
}

DiscordClient::type_signal_reaction_add DiscordClient::signal_reaction_add() {
    return m_signal_reaction_add;
}

DiscordClient::type_signal_reaction_remove DiscordClient::signal_reaction_remove() {
    return m_signal_reaction_remove;
}

DiscordClient::type_signal_typing_start DiscordClient::signal_typing_start() {
    return m_signal_typing_start;
}

DiscordClient::type_signal_guild_member_update DiscordClient::signal_guild_member_update() {
    return m_signal_guild_member_update;
}

DiscordClient::type_signal_guild_ban_remove DiscordClient::signal_guild_ban_remove() {
    return m_signal_guild_ban_remove;
}

DiscordClient::type_signal_guild_ban_add DiscordClient::signal_guild_ban_add() {
    return m_signal_guild_ban_add;
}

DiscordClient::type_signal_invite_create DiscordClient::signal_invite_create() {
    return m_signal_invite_create;
}

DiscordClient::type_signal_invite_delete DiscordClient::signal_invite_delete() {
    return m_signal_invite_delete;
}

DiscordClient::type_signal_presence_update DiscordClient::signal_presence_update() {
    return m_signal_presence_update;
}

DiscordClient::type_signal_note_update DiscordClient::signal_note_update() {
    return m_signal_note_update;
}

DiscordClient::type_signal_guild_emojis_update DiscordClient::signal_guild_emojis_update() {
    return m_signal_guild_emojis_update;
}

DiscordClient::type_signal_guild_join_request_create DiscordClient::signal_guild_join_request_create() {
    return m_signal_guild_join_request_create;
}

DiscordClient::type_signal_guild_join_request_update DiscordClient::signal_guild_join_request_update() {
    return m_signal_guild_join_request_update;
}

DiscordClient::type_signal_guild_join_request_delete DiscordClient::signal_guild_join_request_delete() {
    return m_signal_guild_join_request_delete;
}

DiscordClient::type_signal_relationship_remove DiscordClient::signal_relationship_remove() {
    return m_signal_relationship_remove;
}

DiscordClient::type_signal_relationship_add DiscordClient::signal_relationship_add() {
    return m_signal_relationship_add;
}

DiscordClient::type_signal_message_unpinned DiscordClient::signal_message_unpinned() {
    return m_signal_message_unpinned;
}

DiscordClient::type_signal_message_pinned DiscordClient::signal_message_pinned() {
    return m_signal_message_pinned;
}

DiscordClient::type_signal_thread_create DiscordClient::signal_thread_create() {
    return m_signal_thread_create;
}

DiscordClient::type_signal_thread_delete DiscordClient::signal_thread_delete() {
    return m_signal_thread_delete;
}

DiscordClient::type_signal_thread_list_sync DiscordClient::signal_thread_list_sync() {
    return m_signal_thread_list_sync;
}

DiscordClient::type_signal_thread_members_update DiscordClient::signal_thread_members_update() {
    return m_signal_thread_members_update;
}

DiscordClient::type_signal_thread_update DiscordClient::signal_thread_update() {
    return m_signal_thread_update;
}

DiscordClient::type_signal_thread_member_list_update DiscordClient::signal_thread_member_list_update() {
    return m_signal_thread_member_list_update;
}

DiscordClient::type_signal_message_ack DiscordClient::signal_message_ack() {
    return m_signal_message_ack;
}

DiscordClient::type_signal_guild_members_chunk DiscordClient::signal_guild_members_chunk() {
    return m_signal_guild_members_chunk;
}

DiscordClient::type_signal_added_to_thread DiscordClient::signal_added_to_thread() {
    return m_signal_added_to_thread;
}

DiscordClient::type_signal_removed_from_thread DiscordClient::signal_removed_from_thread() {
    return m_signal_removed_from_thread;
}

DiscordClient::type_signal_message_sent DiscordClient::signal_message_sent() {
    return m_signal_message_sent;
}

DiscordClient::type_signal_channel_muted DiscordClient::signal_channel_muted() {
    return m_signal_channel_muted;
}

DiscordClient::type_signal_channel_unmuted DiscordClient::signal_channel_unmuted() {
    return m_signal_channel_unmuted;
}

DiscordClient::type_signal_guild_muted DiscordClient::signal_guild_muted() {
    return m_signal_guild_muted;
}

DiscordClient::type_signal_guild_unmuted DiscordClient::signal_guild_unmuted() {
    return m_signal_guild_unmuted;
}

DiscordClient::type_signal_channel_accessibility_changed DiscordClient::signal_channel_accessibility_changed() {
    return m_signal_channel_accessibility_changed;
}

DiscordClient::type_signal_message_send_fail DiscordClient::signal_message_send_fail() {
    return m_signal_message_send_fail;
}

#ifdef WITH_VOICE
DiscordClient::type_signal_voice_connected DiscordClient::signal_voice_connected() {
    return m_signal_voice_connected;
}

DiscordClient::type_signal_voice_disconnected DiscordClient::signal_voice_disconnected() {
    return m_signal_voice_disconnected;
}

DiscordClient::type_signal_voice_speaking DiscordClient::signal_voice_speaking() {
    return m_signal_voice_speaking;
}

DiscordClient::type_signal_voice_user_disconnect DiscordClient::signal_voice_user_disconnect() {
    return m_signal_voice_user_disconnect;
}

DiscordClient::type_signal_voice_user_connect DiscordClient::signal_voice_user_connect() {
    return m_signal_voice_user_connect;
}

DiscordClient::type_signal_voice_requested_connect DiscordClient::signal_voice_requested_connect() {
    return m_signal_voice_requested_connect;
}

DiscordClient::type_signal_voice_requested_disconnect DiscordClient::signal_voice_requested_disconnect() {
    return m_signal_voice_requested_disconnect;
}

DiscordClient::type_signal_voice_client_state_update DiscordClient::signal_voice_client_state_update() {
    return m_signal_voice_client_state_update;
}

DiscordClient::type_signal_voice_channel_changed DiscordClient::signal_voice_channel_changed() {
    return m_signal_voice_channel_changed;
}

DiscordClient::type_signal_voice_state_set DiscordClient::signal_voice_state_set() {
    return m_signal_voice_state_set;
}
#endif
