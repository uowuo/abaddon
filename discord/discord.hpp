#pragma once
#include "websocket.hpp"
#include "http.hpp"
#include "objects.hpp"
#include "store.hpp"
#include <sigc++/sigc++.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <mutex>
#include <zlib.h>
#include <glibmm.h>
#include <queue>

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
    static const constexpr char *DiscordGateway = "wss://gateway.discord.gg/?v=8&encoding=json&compress=zlib-stream";
    static const constexpr char *DiscordAPI = "https://discord.com/api";
    static const constexpr char *GatewayIdentity = "Discord";

public:
    DiscordClient();
    void Start();
    void Stop();
    bool IsStarted() const;

    using guilds_type = Store::guilds_type;
    using channels_type = Store::channels_type;
    using messages_type = Store::messages_type;
    using users_type = Store::users_type;
    using roles_type = Store::roles_type;
    using members_type = Store::members_type;
    using permission_overwrites_type = Store::permission_overwrites_type;

    std::unordered_set<Snowflake> GetGuildsID() const;
    const guilds_type &GetGuilds() const;
    const User &GetUserData() const;
    const UserSettings &GetUserSettings() const;
    std::vector<Snowflake> GetUserSortedGuilds() const;
    std::set<Snowflake> GetMessagesForChannel(Snowflake id) const;
    std::set<Snowflake> GetPrivateChannels() const;

    void FetchInviteData(std::string code, std::function<void(Invite)> cb, std::function<void(bool)> err);
    void UpdateSettingsGuildPositions(const std::vector<Snowflake> &pos);
    void FetchMessagesInChannel(Snowflake id, std::function<void(const std::vector<Snowflake> &)> cb);
    void FetchMessagesInChannelBefore(Snowflake channel_id, Snowflake before_id, std::function<void(const std::vector<Snowflake> &)> cb);
    const Message *GetMessage(Snowflake id) const;
    const Channel *GetChannel(Snowflake id) const;
    const User *GetUser(Snowflake id) const;
    const Role *GetRole(Snowflake id) const;
    const Guild *GetGuild(Snowflake id) const;
    const GuildMember *GetMember(Snowflake user_id, Snowflake guild_id) const;
    const PermissionOverwrite *GetPermissionOverwrite(Snowflake channel_id, Snowflake id) const;
    const Emoji *GetEmoji(Snowflake id) const;
    Snowflake GetMemberHoistedRole(Snowflake guild_id, Snowflake user_id, bool with_color = false) const;
    Snowflake GetMemberHighestRole(Snowflake guild_id, Snowflake user_id) const;
    std::unordered_set<Snowflake> GetUsersInGuild(Snowflake id) const;
    std::unordered_set<Snowflake> GetRolesInGuild(Snowflake id) const;
    std::unordered_set<Snowflake> GetChannelsInGuild(Snowflake id) const;

    bool HasGuildPermission(Snowflake user_id, Snowflake guild_id, Permission perm) const;
    bool HasChannelPermission(Snowflake user_id, Snowflake channel_id, Permission perm) const;
    Permission ComputePermissions(Snowflake member_id, Snowflake guild_id) const;
    Permission ComputeOverwrites(Permission base, Snowflake member_id, Snowflake channel_id) const;
    bool CanManageMember(Snowflake guild_id, Snowflake actor, Snowflake target) const; // kick, ban, edit nickname (cant think of a better name)

    void SendChatMessage(std::string content, Snowflake channel);
    void DeleteMessage(Snowflake channel_id, Snowflake id);
    void EditMessage(Snowflake channel_id, Snowflake id, std::string content);
    void SendLazyLoad(Snowflake id);
    void JoinGuild(std::string code);
    void LeaveGuild(Snowflake id);
    void KickUser(Snowflake user_id, Snowflake guild_id);
    void BanUser(Snowflake user_id, Snowflake guild_id); // todo: reason, delete messages
    void UpdateStatus(const std::string &status, bool is_afk, const Activity &obj);
    void CreateDM(Snowflake user_id);
    std::optional<Snowflake> FindDM(Snowflake user_id); // wont find group dms

    void UpdateToken(std::string token);

private:
    static const constexpr int InflateChunkSize = 0x10000;
    std::vector<uint8_t> m_compressed_buf;
    std::vector<uint8_t> m_decompress_buf;
    z_stream m_zstream;

    void ProcessNewGuild(Guild &guild);

    void HandleGatewayMessageRaw(std::string str);
    void HandleGatewayMessage(std::string str);
    void HandleGatewayReady(const GatewayMessage &msg);
    void HandleGatewayMessageCreate(const GatewayMessage &msg);
    void HandleGatewayMessageDelete(const GatewayMessage &msg);
    void HandleGatewayMessageUpdate(const GatewayMessage &msg);
    void HandleGatewayGuildMemberListUpdate(const GatewayMessage &msg);
    void HandleGatewayGuildCreate(const GatewayMessage &msg);
    void HandleGatewayGuildDelete(const GatewayMessage &msg);
    void HandleGatewayMessageDeleteBulk(const GatewayMessage &msg);
    void HandleGatewayGuildMemberUpdate(const GatewayMessage &msg);
    void HandleGatewayPresenceUpdate(const GatewayMessage &msg);
    void HandleGatewayChannelDelete(const GatewayMessage &msg);
    void HandleGatewayChannelUpdate(const GatewayMessage &msg);
    void HandleGatewayChannelCreate(const GatewayMessage &msg);
    void HandleGatewayGuildUpdate(const GatewayMessage &msg);
    void HeartbeatThread();
    void SendIdentify();

    bool CheckCode(const cpr::Response &r);

    std::string m_token;

    void AddMessageToChannel(Snowflake msg_id, Snowflake channel_id);
    std::unordered_map<Snowflake, std::unordered_set<Snowflake>> m_chan_to_message_map;

    void AddUserToGuild(Snowflake user_id, Snowflake guild_id);
    std::unordered_map<Snowflake, std::unordered_set<Snowflake>> m_guild_to_users;

    std::unordered_map<Snowflake, std::unordered_set<Snowflake>> m_guild_to_channels;

    User m_user_data;
    UserSettings m_user_settings;

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

    mutable std::mutex m_msg_mutex;
    Glib::Dispatcher m_msg_dispatch;
    std::queue<std::string> m_msg_queue;
    void MessageDispatch();

    // signals
public:
    typedef sigc::signal<void> type_signal_gateway_ready;
    typedef sigc::signal<void> type_signal_channel_list_refresh;
    typedef sigc::signal<void, Snowflake> type_signal_message_create;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_message_delete;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_message_update;
    typedef sigc::signal<void, Snowflake> type_signal_guild_member_list_update;
    typedef sigc::signal<void, Snowflake> type_signal_guild_create;
    typedef sigc::signal<void, Snowflake> type_signal_guild_delete;
    typedef sigc::signal<void, Snowflake> type_signal_channel_delete;
    typedef sigc::signal<void, Snowflake> type_signal_channel_update;
    typedef sigc::signal<void, Snowflake> type_signal_channel_create;
    typedef sigc::signal<void, Snowflake> type_signal_guild_update;

    type_signal_gateway_ready signal_gateway_ready();
    type_signal_channel_list_refresh signal_channel_list_refresh();
    type_signal_message_create signal_message_create();
    type_signal_message_delete signal_message_delete();
    type_signal_message_update signal_message_update();
    type_signal_guild_member_list_update signal_guild_member_list_update();
    type_signal_guild_create signal_guild_create();
    type_signal_guild_delete signal_guild_delete();
    type_signal_channel_delete signal_channel_delete();
    type_signal_channel_update signal_channel_update();
    type_signal_channel_create signal_channel_create();
    type_signal_guild_update signal_guild_update();

protected:
    type_signal_gateway_ready m_signal_gateway_ready;
    type_signal_channel_list_refresh m_signal_channel_list_refresh;
    type_signal_message_create m_signal_message_create;
    type_signal_message_delete m_signal_message_delete;
    type_signal_message_update m_signal_message_update;
    type_signal_guild_member_list_update m_signal_guild_member_list_update;
    type_signal_guild_create m_signal_guild_create;
    type_signal_guild_delete m_signal_guild_delete;
    type_signal_channel_delete m_signal_channel_delete;
    type_signal_channel_update m_signal_channel_update;
    type_signal_channel_create m_signal_channel_create;
    type_signal_guild_update m_signal_guild_update;
};
