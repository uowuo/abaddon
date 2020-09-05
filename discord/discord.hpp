#pragma once
#include "websocket.hpp"
#include "http.hpp"
#include "objects.hpp"
#include <nlohmann/json.hpp>
#include <thread>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <mutex>
#include <zlib.h>

// bruh
#ifdef GetMessage
    #undef GetMessage
#endif

// https://stackoverflow.com/questions/29775153/stopping-long-sleep-threads/29775639#29775639
class HeartbeatWaiter {
public:
    template<class R, class P>
    bool wait_for(std::chrono::duration<R, P> const &time) const {
        std::unique_lock<std::mutex> lock(m);
        return !cv.wait_for(lock, time, [&] { return terminate; });
    }

    void kill() {
        std::unique_lock<std::mutex> lock(m);
        terminate = true;
        cv.notify_all();
    }

private:
    mutable std::condition_variable cv;
    mutable std::mutex m;
    bool terminate = false;
};

class Abaddon;
class DiscordClient {
    friend class Abaddon;

public:
    static const constexpr char *DiscordGateway = "wss://gateway.discord.gg/?v=6&encoding=json&compress=zlib-stream";
    static const constexpr char *DiscordAPI = "https://discord.com/api";
    static const constexpr char *GatewayIdentity = "Discord";

public:
    DiscordClient();
    void SetAbaddon(Abaddon *ptr);
    void Start();
    void Stop();
    bool IsStarted() const;

    using Channels_t = std::unordered_map<Snowflake, ChannelData>;
    using Guilds_t = std::unordered_map<Snowflake, GuildData>;
    using Messages_t = std::unordered_map<Snowflake, MessageData>;
    using Users_t = std::unordered_map<Snowflake, UserData>;

    const Guilds_t &GetGuilds() const;
    const UserData &GetUserData() const;
    const UserSettingsData &GetUserSettings() const;
    std::vector<std::pair<Snowflake, GuildData>> GetUserSortedGuilds() const;
    std::set<Snowflake> GetMessagesForChannel(Snowflake id) const;
    std::set<Snowflake> GetPrivateChannels() const;

    void UpdateSettingsGuildPositions(const std::vector<Snowflake> &pos);
    void FetchMessagesInChannel(Snowflake id, std::function<void(const std::vector<Snowflake> &)> cb);
    void FetchMessagesInChannelBefore(Snowflake channel_id, Snowflake before_id, std::function<void(const std::vector<Snowflake> &)> cb);
    const MessageData *GetMessage(Snowflake id) const;
    const ChannelData *GetChannel(Snowflake id) const;
    const UserData *GetUser(Snowflake id) const;
    std::unordered_set<Snowflake> GetUsersInGuild(Snowflake id) const;

    void SendChatMessage(std::string content, Snowflake channel);
    void DeleteMessage(Snowflake channel_id, Snowflake id);
    void EditMessage(Snowflake channel_id, Snowflake id, std::string content);
    void SendLazyLoad(Snowflake id);

    void UpdateToken(std::string token);

private:
    static const constexpr int InflateChunkSize = 0x10000;
    std::vector<uint8_t> m_compressed_buf;
    std::vector<uint8_t> m_decompress_buf;
    z_stream m_zstream;
    void HandleGatewayMessageRaw(std::string str);
    void HandleGatewayMessage(std::string str);
    void HandleGatewayReady(const GatewayMessage &msg);
    void HandleGatewayMessageCreate(const GatewayMessage &msg);
    void HandleGatewayMessageDelete(const GatewayMessage &msg);
    void HandleGatewayMessageUpdate(const GatewayMessage &msg);
    void HandleGatewayGuildMemberListUpdate(const GatewayMessage &msg);
    void HeartbeatThread();
    void SendIdentify();

    Abaddon *m_abaddon = nullptr;
    HTTPClient m_http;

    std::string m_token;

    mutable std::mutex m_mutex;

    void StoreGuild(Snowflake id, const GuildData &g);
    Guilds_t m_guilds;

    void StoreMessage(Snowflake id, const MessageData &m);
    Messages_t m_messages;
    std::unordered_map<Snowflake, std::unordered_set<const MessageData *>> m_chan_to_message_map;

    void StoreChannel(Snowflake id, const ChannelData &c);
    Channels_t m_channels;

    void AddUserToGuild(Snowflake user_id, Snowflake guild_id);
    void StoreUser(const UserData &u);
    Users_t m_users;
    std::unordered_map<Snowflake, std::unordered_set<Snowflake>> m_guild_to_users;

    UserData m_user_data;
    UserSettingsData m_user_settings;

    Websocket m_websocket;
    std::atomic<bool> m_client_connected = false;
    std::atomic<bool> m_ready_received = false;
    std::unordered_map<std::string, GatewayEvent> m_event_map;
    void LoadEventMap();

    std::thread m_heartbeat_thread;
    int m_last_sequence = -1;
    int m_heartbeat_msec = 0;
    HeartbeatWaiter m_heartbeat_waiter;
    bool m_heartbeat_acked = true;
};
