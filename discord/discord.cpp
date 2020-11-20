#include "discord.hpp"
#include <cassert>
#include "../util.hpp"

DiscordClient::DiscordClient()
    : m_http(DiscordAPI)
    , m_decompress_buf(InflateChunkSize) {
    m_msg_dispatch.connect(sigc::mem_fun(*this, &DiscordClient::MessageDispatch));

    m_websocket.signal_message().connect(sigc::mem_fun(*this, &DiscordClient::HandleGatewayMessageRaw));
    m_websocket.signal_open().connect(sigc::mem_fun(*this, &DiscordClient::HandleSocketOpen));
    m_websocket.signal_close().connect(sigc::mem_fun(*this, &DiscordClient::HandleSocketClose));

    LoadEventMap();
}

void DiscordClient::Start() {
    std::memset(&m_zstream, 0, sizeof(m_zstream));
    inflateInit2(&m_zstream, MAX_WBITS + 32);

    m_last_sequence = -1;
    m_heartbeat_acked = true;
    m_client_connected = true;
    m_websocket.StartConnection(DiscordGateway);
}

void DiscordClient::Stop() {
    if (!m_client_connected) return;

    inflateEnd(&m_zstream);
    m_compressed_buf.clear();

    m_heartbeat_waiter.kill();
    if (m_heartbeat_thread.joinable()) m_heartbeat_thread.join();
    m_client_connected = false;

    m_store.ClearAll();
    m_chan_to_message_map.clear();
    m_guild_to_users.clear();

    m_websocket.Stop();

    m_signal_disconnected.emit(false);
}

bool DiscordClient::IsStarted() const {
    return m_client_connected;
}

bool DiscordClient::IsStoreValid() const {
    return m_store.IsValid();
}

std::unordered_set<Snowflake> DiscordClient::GetGuildsID() const {
    const auto &guilds = m_store.GetGuilds();
    std::unordered_set<Snowflake> ret;
    for (const auto &[gid, data] : guilds)
        ret.insert(gid);
    return ret;
}

const Store::guilds_type &DiscordClient::GetGuilds() const {
    return m_store.GetGuilds();
}

const UserSettings &DiscordClient::GetUserSettings() const {
    return m_user_settings;
}

const User &DiscordClient::GetUserData() const {
    return m_user_data;
}

std::vector<Snowflake> DiscordClient::GetUserSortedGuilds() const {
    // sort order is unfolder'd guilds sorted by id descending, then guilds in folders in array order
    // todo: make sure folder'd guilds are sorted properly
    std::vector<Snowflake> folder_order;
    auto guilds = GetGuildsID();
    for (const auto &entry : m_user_settings.GuildFolders) { // can contain guilds not a part of
        for (const auto &id : entry.GuildIDs) {
            if (std::find(guilds.begin(), guilds.end(), id) != guilds.end())
                folder_order.push_back(id);
        }
    }
    std::vector<Snowflake> ret;
    for (const auto &gid : guilds) {
        if (std::find(folder_order.begin(), folder_order.end(), gid) == folder_order.end()) {
            ret.push_back(gid);
        }
    }
    for (const auto &gid : folder_order)
        ret.push_back(gid);
    return ret;
}

std::set<Snowflake> DiscordClient::GetMessagesForChannel(Snowflake id) const {
    auto it = m_chan_to_message_map.find(id);
    if (it == m_chan_to_message_map.end())
        return std::set<Snowflake>();

    std::set<Snowflake> ret;
    for (const auto &msg_id : it->second)
        ret.insert(m_store.GetMessage(msg_id)->ID);

    return ret;
}

void DiscordClient::FetchInviteData(std::string code, std::function<void(Invite)> cb, std::function<void(bool)> err) {
    m_http.MakeGET("/invites/" + code + "?with_counts=true", [this, cb, err](cpr::Response r) {
        if (!CheckCode(r)) {
            err(r.status_code == 404);
            return;
        };

        cb(nlohmann::json::parse(r.text));
    });
}

void DiscordClient::FetchMessagesInChannel(Snowflake id, std::function<void(const std::vector<Snowflake> &)> cb) {
    std::string path = "/channels/" + std::to_string(id) + "/messages?limit=50";
    m_http.MakeGET(path, [this, id, cb](cpr::Response r) {
        if (!CheckCode(r)) return;

        std::vector<Message> msgs;
        std::vector<Snowflake> ids;

        nlohmann::json::parse(r.text).get_to(msgs);

        m_store.BeginTransaction();
        for (const auto &msg : msgs) {
            m_store.SetMessage(msg.ID, msg);
            AddMessageToChannel(msg.ID, id);
            m_store.SetUser(msg.Author.ID, msg.Author);
            AddUserToGuild(msg.Author.ID, *msg.GuildID);
            ids.push_back(msg.ID);
        }
        m_store.EndTransaction();

        cb(ids);
    });
}

void DiscordClient::FetchMessagesInChannelBefore(Snowflake channel_id, Snowflake before_id, std::function<void(const std::vector<Snowflake> &)> cb) {
    std::string path = "/channels/" + std::to_string(channel_id) + "/messages?limit=50&before=" + std::to_string(before_id);
    m_http.MakeGET(path, [this, channel_id, cb](cpr::Response r) {
        if (!CheckCode(r)) return;

        std::vector<Message> msgs;
        std::vector<Snowflake> ids;

        nlohmann::json::parse(r.text).get_to(msgs);

        m_store.BeginTransaction();
        for (const auto &msg : msgs) {
            m_store.SetMessage(msg.ID, msg);
            AddMessageToChannel(msg.ID, channel_id);
            m_store.SetUser(msg.Author.ID, msg.Author);
            AddUserToGuild(msg.Author.ID, *msg.GuildID);
            ids.push_back(msg.ID);
        }
        m_store.EndTransaction();

        cb(ids);
    });
}

const Message *DiscordClient::GetMessage(Snowflake id) const {
    return m_store.GetMessage(id);
}

const Channel *DiscordClient::GetChannel(Snowflake id) const {
    return m_store.GetChannel(id);
}

std::optional<User> DiscordClient::GetUser(Snowflake id) const {
    return m_store.GetUser(id);
}

const Role *DiscordClient::GetRole(Snowflake id) const {
    return m_store.GetRole(id);
}

const Guild *DiscordClient::GetGuild(Snowflake id) const {
    return m_store.GetGuild(id);
}

const GuildMember *DiscordClient::GetMember(Snowflake user_id, Snowflake guild_id) const {
    return m_store.GetGuildMemberData(guild_id, user_id);
}

const PermissionOverwrite *DiscordClient::GetPermissionOverwrite(Snowflake channel_id, Snowflake id) const {
    return m_store.GetPermissionOverwrite(channel_id, id);
}

const Emoji *DiscordClient::GetEmoji(Snowflake id) const {
    return m_store.GetEmoji(id);
}

Snowflake DiscordClient::GetMemberHoistedRole(Snowflake guild_id, Snowflake user_id, bool with_color) const {
    auto *data = m_store.GetGuildMemberData(guild_id, user_id);
    if (data == nullptr) return Snowflake::Invalid;

    std::vector<const Role *> roles;
    for (const auto &id : data->Roles) {
        auto *role = GetRole(id);
        if (role != nullptr) {
            if (role->IsHoisted || (with_color && role->Color != 0))
                roles.push_back(role);
        }
    }

    if (roles.size() == 0) return Snowflake::Invalid;

    std::sort(roles.begin(), roles.end(), [this](const Role *a, const Role *b) -> bool {
        return a->Position > b->Position;
    });

    return roles[0]->ID;
}

Snowflake DiscordClient::GetMemberHighestRole(Snowflake guild_id, Snowflake user_id) const {
    const auto *data = GetMember(user_id, guild_id);
    if (data == nullptr) return Snowflake::Invalid;

    if (data->Roles.size() == 0) return Snowflake::Invalid;
    if (data->Roles.size() == 1) return data->Roles[0];

    return *std::max(data->Roles.begin(), data->Roles.end(), [this](const auto &a, const auto &b) -> bool {
        const auto *role_a = GetRole(*a);
        const auto *role_b = GetRole(*b);
        if (role_a == nullptr || role_b == nullptr) return false; // for some reason a Snowflake(0) sneaks into here
        return role_a->Position < role_b->Position;
    });
}

std::unordered_set<Snowflake> DiscordClient::GetUsersInGuild(Snowflake id) const {
    auto it = m_guild_to_users.find(id);
    if (it != m_guild_to_users.end())
        return it->second;

    return std::unordered_set<Snowflake>();
}

std::unordered_set<Snowflake> DiscordClient::GetRolesInGuild(Snowflake id) const {
    std::unordered_set<Snowflake> ret;
    const auto &roles = m_store.GetRoles();
    for (const auto &[rid, rdata] : roles)
        ret.insert(rid);
    return ret;
}

std::unordered_set<Snowflake> DiscordClient::GetChannelsInGuild(Snowflake id) const {
    auto it = m_guild_to_channels.find(id);
    if (it != m_guild_to_channels.end())
        return it->second;
    return std::unordered_set<Snowflake>();
}

bool DiscordClient::HasGuildPermission(Snowflake user_id, Snowflake guild_id, Permission perm) const {
    const auto base = ComputePermissions(user_id, guild_id);
    return (base & perm) == perm;
}

bool DiscordClient::HasChannelPermission(Snowflake user_id, Snowflake channel_id, Permission perm) const {
    const auto *channel = m_store.GetChannel(channel_id);
    if (channel == nullptr) return false;
    const auto base = ComputePermissions(user_id, channel->GuildID);
    const auto overwrites = ComputeOverwrites(base, user_id, channel_id);
    return (overwrites & perm) == perm;
}

Permission DiscordClient::ComputePermissions(Snowflake member_id, Snowflake guild_id) const {
    const auto *member = GetMember(member_id, guild_id);
    const auto *guild = GetGuild(guild_id);
    if (member == nullptr || guild == nullptr)
        return Permission::NONE;

    if (guild->OwnerID == member_id)
        return Permission::ALL;

    const auto *everyone = GetRole(guild_id);
    if (everyone == nullptr)
        return Permission::NONE;

    Permission perms = everyone->Permissions;
    for (const auto role_id : member->Roles) {
        const auto *role = GetRole(role_id);
        if (role != nullptr)
            perms |= role->Permissions;
    }

    if ((perms & Permission::ADMINISTRATOR) == Permission::ADMINISTRATOR)
        return Permission::ALL;

    return perms;
}

Permission DiscordClient::ComputeOverwrites(Permission base, Snowflake member_id, Snowflake channel_id) const {
    if ((base & Permission::ADMINISTRATOR) == Permission::ADMINISTRATOR)
        return Permission::ALL;

    const auto *channel = GetChannel(channel_id);
    const auto *member = GetMember(member_id, channel->GuildID);
    if (member == nullptr || channel == nullptr)
        return Permission::NONE;

    Permission perms = base;
    const auto *overwrite_everyone = GetPermissionOverwrite(channel_id, channel->GuildID);
    if (overwrite_everyone != nullptr) {
        perms &= ~overwrite_everyone->Deny;
        perms |= overwrite_everyone->Allow;
    }

    Permission allow = Permission::NONE;
    Permission deny = Permission::NONE;
    for (const auto role_id : member->Roles) {
        const auto *overwrite = GetPermissionOverwrite(channel_id, role_id);
        if (overwrite != nullptr) {
            allow |= overwrite->Allow;
            deny |= overwrite->Deny;
        }
    }

    perms &= ~deny;
    perms |= allow;

    const auto *member_overwrite = GetPermissionOverwrite(channel_id, member_id);
    if (member_overwrite != nullptr) {
        perms &= ~member_overwrite->Deny;
        perms |= member_overwrite->Allow;
    }

    return perms;
}

bool DiscordClient::CanManageMember(Snowflake guild_id, Snowflake actor, Snowflake target) const {
    const auto *guild = GetGuild(guild_id);
    if (guild != nullptr && guild->OwnerID == target) return false;
    const auto actor_highest_id = GetMemberHighestRole(guild_id, actor);
    const auto target_highest_id = GetMemberHighestRole(guild_id, target);
    const auto *actor_highest = GetRole(actor_highest_id);
    const auto *target_highest = GetRole(target_highest_id);
    if (actor_highest == nullptr) return false;
    if (target_highest == nullptr) return true;
    return actor_highest->Position > target_highest->Position;
}

void DiscordClient::SendChatMessage(std::string content, Snowflake channel) {
    // @([^@#]{1,32})#(\\d{4})
    CreateMessageObject obj;
    obj.Content = content;
    nlohmann::json j = obj;
    m_http.MakePOST("/channels/" + std::to_string(channel) + "/messages", j.dump(), [](auto) {});
}

void DiscordClient::DeleteMessage(Snowflake channel_id, Snowflake id) {
    std::string path = "/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(id);
    m_http.MakeDELETE(path, [](auto) {});
}

void DiscordClient::EditMessage(Snowflake channel_id, Snowflake id, std::string content) {
    std::string path = "/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(id);
    MessageEditObject obj;
    obj.Content = content;
    nlohmann::json j = obj;
    m_http.MakePATCH(path, j.dump(), [](auto) {});
}

void DiscordClient::SendLazyLoad(Snowflake id) {
    LazyLoadRequestMessage msg;
    std::unordered_map<Snowflake, std::vector<std::pair<int, int>>> c;
    c[id] = {
        std::make_pair(0, 99),
        std::make_pair(100, 199)
    };
    msg.Channels = c;
    msg.GuildID = GetChannel(id)->GuildID;
    msg.ShouldGetActivities = false;
    msg.ShouldGetTyping = false;

    nlohmann::json j = msg;
    m_websocket.Send(j);
}

void DiscordClient::JoinGuild(std::string code) {
    m_http.MakePOST("/invites/" + code, "", [](auto) {});
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

void DiscordClient::UpdateStatus(const std::string &status, bool is_afk, const Activity &obj) {
    UpdateStatusMessage msg;
    msg.Status = status;
    msg.IsAFK = is_afk;
    msg.Activities.push_back(obj);

    m_websocket.Send(nlohmann::json(msg));
}

void DiscordClient::CreateDM(Snowflake user_id) {
    // actual client uses an array called recipients
    CreateDMObject obj;
    obj.Recipients.push_back(user_id);
    m_http.MakePOST("/users/@me/channels", nlohmann::json(obj).dump(), [](auto) {});
}

std::optional<Snowflake> DiscordClient::FindDM(Snowflake user_id) {
    const auto &channels = m_store.GetChannels();
    for (const auto &[id, channel] : channels) {
        if (channel.Recipients.size() == 1 && channel.Recipients[0].ID == user_id)
            return id;
    }

    return std::nullopt;
}

void DiscordClient::UpdateToken(std::string token) {
    if (!IsStarted()) {
        m_token = token;
        m_http.SetAuth(token);
    }
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
                }
            } break;
            default:
                printf("Unknown opcode %d\n", m.Opcode);
                break;
        }
    } catch (std::exception &e) {
        fprintf(stderr, "error handling message (opcode %d): %s\n", m.Opcode, e.what());
    }
}

void DiscordClient::HandleGatewayHello(const GatewayMessage &msg) {
    HelloMessageData d = msg.Data;
    m_heartbeat_msec = d.HeartbeatInterval;
    m_heartbeat_waiter.revive();
    m_heartbeat_thread = std::thread(std::bind(&DiscordClient::HeartbeatThread, this));
    m_signal_connected.emit(); // socket is connected before this but emitting here should b fine
    if (m_wants_resume) {
        m_wants_resume = false;
        SendResume();
    } else
        SendIdentify();
}

void DiscordClient::ProcessNewGuild(Guild &guild) {
    if (guild.IsUnavailable) {
        printf("guild (%lld) unavailable\n", static_cast<uint64_t>(guild.ID));
        return;
    }

    m_store.SetGuild(guild.ID, guild);
    for (auto &c : guild.Channels) {
        c.GuildID = guild.ID;
        m_store.SetChannel(c.ID, c);
        m_guild_to_channels[guild.ID].insert(c.ID);
        for (auto &p : c.PermissionOverwrites) {
            m_store.SetPermissionOverwrite(c.ID, p.ID, p);
        }
    }

    for (auto &r : guild.Roles)
        m_store.SetRole(r.ID, r);

    for (auto &e : guild.Emojis)
        m_store.SetEmoji(e.ID, e);
}

void DiscordClient::HandleGatewayReady(const GatewayMessage &msg) {
    m_ready_received = true;
    ReadyEventData data = msg.Data;
    for (auto &g : data.Guilds)
        ProcessNewGuild(g);

    m_store.BeginTransaction();
    for (const auto &dm : data.PrivateChannels) {
        m_store.SetChannel(dm.ID, dm);
        for (const auto &recipient : dm.Recipients)
            m_store.SetUser(recipient.ID, recipient);
    }
    m_store.EndTransaction();

    m_session_id = data.SessionID;
    m_user_data = data.User;
    m_user_settings = data.UserSettings;
    m_signal_gateway_ready.emit();
}

void DiscordClient::HandleGatewayMessageCreate(const GatewayMessage &msg) {
    Message data = msg.Data;
    m_store.SetMessage(data.ID, data);
    AddMessageToChannel(data.ID, data.ChannelID);
    m_store.SetUser(data.Author.ID, data.Author);
    AddUserToGuild(data.Author.ID, *data.GuildID);
    m_signal_message_create.emit(data.ID);
}

void DiscordClient::HandleGatewayMessageDelete(const GatewayMessage &msg) {
    MessageDeleteData data = msg.Data;
    auto *cur = m_store.GetMessage(data.ID);
    if (cur != nullptr)
        cur->SetDeleted();
    m_signal_message_delete.emit(data.ID, data.ChannelID);
}

void DiscordClient::HandleGatewayMessageDeleteBulk(const GatewayMessage &msg) {
    MessageDeleteBulkData data = msg.Data;
    for (const auto &id : data.IDs) {
        auto *cur = m_store.GetMessage(id);
        if (cur != nullptr)
            cur->SetDeleted();
        m_signal_message_delete.emit(id, data.ChannelID);
    }
}

void DiscordClient::HandleGatewayGuildMemberUpdate(const GatewayMessage &msg) {
    GuildMemberUpdateMessage data = msg.Data;
    auto member = GuildMember::from_update_json(msg.Data); // meh
    m_store.SetGuildMemberData(data.GuildID, data.User.ID, member);
}

void DiscordClient::HandleGatewayPresenceUpdate(const GatewayMessage &msg) {
    PresenceUpdateMessage data = msg.Data;
    auto cur = m_store.GetUser(data.User.at("id").get<Snowflake>());
    if (cur.has_value()) {
        User::update_from_json(data.User, *cur);
        m_store.SetUser(cur->ID, *cur);
    }
}

void DiscordClient::HandleGatewayChannelDelete(const GatewayMessage &msg) {
    const auto id = msg.Data.at("id").get<Snowflake>();
    const auto *channel = GetChannel(id);
    auto it = m_guild_to_channels.find(channel->GuildID);
    if (it != m_guild_to_channels.end())
        it->second.erase(id);
    m_store.ClearChannel(id);
    m_signal_channel_delete.emit(id);
}

void DiscordClient::HandleGatewayChannelUpdate(const GatewayMessage &msg) {
    const auto id = msg.Data.at("id").get<Snowflake>();
    auto *cur = m_store.GetChannel(id);
    if (cur != nullptr) {
        cur->update_from_json(msg.Data);
        m_signal_channel_update.emit(id);
    }
}

void DiscordClient::HandleGatewayChannelCreate(const GatewayMessage &msg) {
    Channel data = msg.Data;
    m_store.SetChannel(data.ID, data);
    m_guild_to_channels[data.GuildID].insert(data.ID);
    for (const auto &p : data.PermissionOverwrites)
        m_store.SetPermissionOverwrite(data.ID, p.ID, p);
    m_signal_channel_create.emit(data.ID);
}

void DiscordClient::HandleGatewayGuildUpdate(const GatewayMessage &msg) {
    Snowflake id = msg.Data.at("id");
    auto *current = m_store.GetGuild(id);
    if (current == nullptr) return;
    current->update_from_json(msg.Data);
    m_signal_guild_update.emit(id);
}

void DiscordClient::HandleGatewayReconnect(const GatewayMessage &msg) {
    m_signal_disconnected.emit(true);
    inflateEnd(&m_zstream);
    m_compressed_buf.clear();

    m_heartbeat_waiter.kill();
    if (m_heartbeat_thread.joinable()) m_heartbeat_thread.join();

    m_websocket.Stop(1012); // 1000 (kNormalClosureCode) and 1001 will invalidate the session id

    std::memset(&m_zstream, 0, sizeof(m_zstream));
    inflateInit2(&m_zstream, MAX_WBITS + 32);

    m_heartbeat_acked = true;
    m_wants_resume = true;
    m_websocket.StartConnection(DiscordGateway);
}

void DiscordClient::HandleGatewayMessageUpdate(const GatewayMessage &msg) {
    Snowflake id = msg.Data.at("id");

    auto *current = m_store.GetMessage(id);
    if (current == nullptr)
        return;

    current->from_json_edited(msg.Data);

    m_signal_message_update.emit(id, current->ChannelID);
}

void DiscordClient::HandleGatewayGuildMemberListUpdate(const GatewayMessage &msg) {
    GuildMemberListUpdateMessage data = msg.Data;

    m_store.BeginTransaction();

    bool has_sync = false;
    for (const auto &op : data.Ops) {
        if (op.Op == "SYNC") {
            has_sync = true;
            for (const auto &item : op.Items) {
                if (item->Type == "member") {
                    auto member = static_cast<const GuildMemberListUpdateMessage::MemberItem *>(item.get());
                    m_store.SetUser(member->User.ID, member->User);
                    AddUserToGuild(member->User.ID, data.GuildID);
                    m_store.SetGuildMemberData(data.GuildID, member->User.ID, member->GetAsMemberData());
                }
            }
        }
    }

    m_store.EndTransaction();

    // todo: manage this event a little better
    if (has_sync)
        m_signal_guild_member_list_update.emit(data.GuildID);
}

void DiscordClient::HandleGatewayGuildCreate(const GatewayMessage &msg) {
    Guild data = msg.Data;
    ProcessNewGuild(data);

    m_signal_guild_create.emit(data.ID);
}

void DiscordClient::HandleGatewayGuildDelete(const GatewayMessage &msg) {
    Snowflake id = msg.Data.at("id");
    bool unavailable = msg.Data.contains("unavilable") && msg.Data.at("unavailable").get<bool>();

    if (unavailable)
        printf("guild %llu became unavailable\n", static_cast<uint64_t>(id));

    auto *guild = m_store.GetGuild(id);
    if (guild == nullptr) {
        m_store.ClearGuild(id);
        m_signal_guild_delete.emit(id);
        return;
    }

    m_store.ClearGuild(id);
    for (const auto &c : guild->Channels)
        m_store.ClearChannel(c.ID);

    m_signal_guild_delete.emit(id);
}

void DiscordClient::AddMessageToChannel(Snowflake msg_id, Snowflake channel_id) {
    m_chan_to_message_map[channel_id].insert(msg_id);
}

void DiscordClient::AddUserToGuild(Snowflake user_id, Snowflake guild_id) {
    m_guild_to_users[guild_id].insert(user_id);
}

std::set<Snowflake> DiscordClient::GetPrivateChannels() const {
    auto ret = std::set<Snowflake>();

    for (const auto &[id, chan] : m_store.GetChannels()) {
        if (chan.Type == ChannelType::DM || chan.Type == ChannelType::GROUP_DM)
            ret.insert(id);
    }

    return ret;
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
    msg.Properties.OS = "OpenBSD";
    msg.Properties.Device = GatewayIdentity;
    msg.Properties.Browser = GatewayIdentity;
    msg.Token = m_token;
    m_websocket.Send(msg);
}

void DiscordClient::SendResume() {
    ResumeMessage msg;
    msg.Sequence = m_last_sequence;
    msg.SessionID = m_session_id;
    msg.Token = m_token;
    m_websocket.Send(msg);
}

void DiscordClient::HandleSocketOpen() {
}

void DiscordClient::HandleSocketClose(uint16_t code) {
}

bool DiscordClient::CheckCode(const cpr::Response &r) {
    if (r.status_code >= 300 || r.error) {
        fprintf(stderr, "api request to %s failed with status code %d\n", r.url.c_str(), r.status_code);
        return false;
    }

    return true;
}

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
}

DiscordClient::type_signal_gateway_ready DiscordClient::signal_gateway_ready() {
    return m_signal_gateway_ready;
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
