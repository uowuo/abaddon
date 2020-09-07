#pragma once
#include "websocket.hpp"
#include "http.hpp"
#include "objects.hpp"
#include "store.hpp"
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

    using guilds_type = Store::guilds_type;
    using channels_type = Store::channels_type;
    using messages_type = Store::messages_type;
    using users_type = Store::users_type;
    using roles_type = Store::roles_type;
    using members_type = Store::members_type;

    const guilds_type &GetGuilds() const;
    const UserData &GetUserData() const;
    const UserSettingsData &GetUserSettings() const;
    std::vector<Snowflake> GetUserSortedGuilds() const;
    std::set<Snowflake> GetMessagesForChannel(Snowflake id) const;
    std::set<Snowflake> GetPrivateChannels() const;

    void UpdateSettingsGuildPositions(const std::vector<Snowflake> &pos);
    void FetchMessagesInChannel(Snowflake id, std::function<void(const std::vector<Snowflake> &)> cb);
    void FetchMessagesInChannelBefore(Snowflake channel_id, Snowflake before_id, std::function<void(const std::vector<Snowflake> &)> cb);
    const MessageData *GetMessage(Snowflake id) const;
    const ChannelData *GetChannel(Snowflake id) const;
    const UserData *GetUser(Snowflake id) const;
    const RoleData *GetRole(Snowflake id) const;
    const GuildData *GetGuild(Snowflake id) const;
    Snowflake GetMemberHoistedRole(Snowflake guild_id, Snowflake user_id, bool with_color = false) const;
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

    bool CheckCode(const cpr::Response &r);

    std::string m_token;

    void AddMessageToChannel(Snowflake msg_id, Snowflake channel_id);
    std::unordered_map<Snowflake, std::unordered_set<Snowflake>> m_chan_to_message_map;

    void AddUserToGuild(Snowflake user_id, Snowflake guild_id);
    std::unordered_map<Snowflake, std::unordered_set<Snowflake>> m_guild_to_users;

    UserData m_user_data;
    UserSettingsData m_user_settings;

    Abaddon *m_abaddon = nullptr;
    Store m_store;
    HTTPClient m_http;
    Websocket m_websocket;
    std::atomic<bool> m_client_connected = false;
    std::atomic<bool> m_ready_received = false;

    std::unordered_map<std::string, GatewayEvent> m_event_map;
    void LoadEventMap();

    std::thread m_heartbeat_thread;
    std::atomic<int> m_last_sequence = -1;
    std::atomic<int> m_heartbeat_msec = 0;
    HeartbeatWaiter m_heartbeat_waiter;
    std::atomic<bool> m_heartbeat_acked = true;
};
