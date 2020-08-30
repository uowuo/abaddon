#include "../abaddon.hpp"
#include "discord.hpp"
#include <cassert>

DiscordClient::DiscordClient()
    : m_http(DiscordAPI)
    , m_decompress_buf(InflateChunkSize) {
    LoadEventMap();
}

void DiscordClient::SetAbaddon(Abaddon *ptr) {
    m_abaddon = ptr;
}

void DiscordClient::Start() {
    assert(!m_client_connected);
    assert(!m_websocket.IsOpen());

    std::memset(&m_zstream, 0, sizeof(m_zstream));
    inflateInit2(&m_zstream, MAX_WBITS + 32);

    m_client_connected = true;
    m_websocket.StartConnection(DiscordGateway);
    m_websocket.SetMessageCallback(std::bind(&DiscordClient::HandleGatewayMessageRaw, this, std::placeholders::_1));
}

void DiscordClient::Stop() {
    std::scoped_lock<std::mutex> guard(m_mutex);
    if (!m_client_connected) return;

    inflateEnd(&m_zstream);

    m_heartbeat_waiter.kill();
    if (m_heartbeat_thread.joinable()) m_heartbeat_thread.join();
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

const UserData &DiscordClient::GetUserData() const {
    std::scoped_lock<std::mutex> guard(m_mutex);
    assert(m_ready_received);
    return m_user_data;
}

std::vector<std::pair<Snowflake, GuildData>> DiscordClient::GetUserSortedGuilds() const {
    std::vector<std::pair<Snowflake, GuildData>> sorted_guilds;

    if (m_user_settings.GuildPositions.size()) {
        std::unordered_set<Snowflake> positioned_guilds(m_user_settings.GuildPositions.begin(), m_user_settings.GuildPositions.end());
        // guilds not in the guild_positions object are at the top of the list, descending by guild ID
        std::set<Snowflake> unpositioned_guilds;
        for (const auto &[id, guild] : m_guilds) {
            if (positioned_guilds.find(id) == positioned_guilds.end())
                unpositioned_guilds.insert(id);
        }

        // unpositioned_guilds now has unpositioned guilds in ascending order
        for (auto it = unpositioned_guilds.rbegin(); it != unpositioned_guilds.rend(); it++)
            if (m_guilds.find(*it) != m_guilds.end())
                sorted_guilds.push_back(std::make_pair(*it, m_guilds.at(*it)));

        // now the rest go at the end in the order they are sorted
        for (const auto &id : m_user_settings.GuildPositions) {
            if (m_guilds.find(id) != m_guilds.end())
                sorted_guilds.push_back(std::make_pair(id, m_guilds.at(id)));
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

void DiscordClient::FetchMessagesInChannelBefore(Snowflake channel_id, Snowflake before_id, std::function<void(const std::vector<MessageData> &)> cb) {
    std::string path = "/channels/" + std::to_string(channel_id) + "/messages?limit=50&before=" + std::to_string(before_id);
    m_http.MakeGET(path, [this, channel_id, cb](cpr::Response r) {
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

const ChannelData *DiscordClient::GetChannel(Snowflake id) const {
    return &m_channels.at(id);
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

void DiscordClient::UpdateToken(std::string token) {
    m_token = token;
    m_http.SetAuth(token);
}

std::string DiscordClient::DecompressGatewayMessage(std::string str) {
    return std::string();
}

void DiscordClient::HandleGatewayMessageRaw(std::string str) {
    // handles multiple zlib compressed messages, calling HandleGatewayMessage when a full message is received
    std::vector<uint8_t> buf(str.begin(), str.end());
    int len = buf.size();
    bool has_suffix = buf[len - 4] == 0x00 && buf[len - 3] == 0x00 && buf[len - 2] == 0xFF && buf[len - 1] == 0xFF;

    m_compressed_buf.insert(m_compressed_buf.end(), buf.begin(), buf.end());

    if (!has_suffix) return;

    m_zstream.next_in = m_compressed_buf.data();
    m_zstream.avail_in = m_compressed_buf.size();
    m_zstream.total_in = m_zstream.total_out = 0;

    // loop in case of really big messages (e.g. READY)
    while (true) {
        m_zstream.next_out = m_decompress_buf.data() + m_zstream.total_out;
        m_zstream.avail_out = m_decompress_buf.size() - m_zstream.total_out;

        int err = inflate(&m_zstream, Z_SYNC_FLUSH);
        if ((err == Z_OK || err == Z_BUF_ERROR) && m_zstream.avail_in > 0) {
            m_decompress_buf.resize(m_decompress_buf.size() + InflateChunkSize);
        } else {
            if (err != Z_OK) {
                fprintf(stderr, "Error decompressing input buffer %d (%d/%d)\n", err, m_zstream.avail_in, m_zstream.avail_out);
            } else {
                HandleGatewayMessage(std::string(m_decompress_buf.begin(), m_decompress_buf.begin() + m_zstream.total_out));
                if (m_decompress_buf.size() > InflateChunkSize)
                    m_decompress_buf.resize(InflateChunkSize);
            }
            break;
        }
    }

    m_compressed_buf.clear();
}

void DiscordClient::HandleGatewayMessage(std::string str) {
    GatewayMessage m;
    try {
        m = nlohmann::json::parse(str);
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
                case GatewayEvent::MESSAGE_DELETE: {
                    HandleGatewayMessageDelete(m);
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
        if (g.IsUnavailable)
            printf("guild (%lld) unavailable\n", g.ID);
        else {
            StoreGuild(g.ID, g);
            for (const auto &c : g.Channels)
                StoreChannel(c.ID, c);
        }
    }

    for (const auto &dm : data.PrivateChannels) {
        StoreChannel(dm.ID, dm);
    }

    m_abaddon->DiscordNotifyReady();
    m_user_data = data.User;
    m_user_settings = data.UserSettings;
}

void DiscordClient::HandleGatewayMessageCreate(const GatewayMessage &msg) {
    MessageData data = msg.Data;
    StoreMessage(data.ID, data);
    m_abaddon->DiscordNotifyMessageCreate(data.ID);
}
void DiscordClient::HandleGatewayMessageDelete(const GatewayMessage &msg) {
    MessageDeleteData data = msg.Data;
    m_abaddon->DiscordNotifyMessageDelete(data.ID, data.ChannelID);
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

void DiscordClient::StoreChannel(Snowflake id, const ChannelData &c) {
    m_channels[id] = c;
}

std::set<Snowflake> DiscordClient::GetPrivateChannels() const {
    auto ret = std::set<Snowflake>();

    for (const auto &[id, chan] : m_channels) {
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
    m_event_map["MESSAGE_DELETE"] = GatewayEvent::MESSAGE_DELETE;
}
