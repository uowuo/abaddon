#pragma once
#include "websocket.hpp"
#include "httpclient.hpp"
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

    void revive() {
        std::unique_lock<std::mutex> lock(m);
        terminate = false;
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
    static const constexpr char *DiscordAPI = "https://discord.com/api/v8";

public:
    DiscordClient(bool mem_store = false);
    void Start();
    void Stop();
    bool IsStarted() const;
    bool IsStoreValid() const;

    using guilds_type = Store::guilds_type;
    using channels_type = Store::channels_type;
    using messages_type = Store::messages_type;
    using users_type = Store::users_type;
    using roles_type = Store::roles_type;
    using members_type = Store::members_type;
    using permission_overwrites_type = Store::permission_overwrites_type;

    std::unordered_set<Snowflake> GetGuilds() const;
    const UserData &GetUserData() const;
    const UserSettings &GetUserSettings() const;
    std::vector<Snowflake> GetUserSortedGuilds() const;
    std::set<Snowflake> GetMessagesForChannel(Snowflake id) const;
    std::set<Snowflake> GetPrivateChannels() const;

    void FetchMessagesInChannel(Snowflake id, std::function<void(const std::vector<Snowflake> &)> cb);
    void FetchMessagesInChannelBefore(Snowflake channel_id, Snowflake before_id, std::function<void(const std::vector<Snowflake> &)> cb);
    std::optional<Message> GetMessage(Snowflake id) const;
    std::optional<ChannelData> GetChannel(Snowflake id) const;
    std::optional<EmojiData> GetEmoji(Snowflake id) const;
    std::optional<PermissionOverwrite> GetPermissionOverwrite(Snowflake channel_id, Snowflake id) const;
    std::optional<UserData> GetUser(Snowflake id) const;
    std::optional<RoleData> GetRole(Snowflake id) const;
    std::optional<GuildData> GetGuild(Snowflake id) const;
    std::optional<GuildMember> GetMember(Snowflake user_id, Snowflake guild_id) const;
    std::optional<BanData> GetBan(Snowflake guild_id, Snowflake user_id) const;
    Snowflake GetMemberHoistedRole(Snowflake guild_id, Snowflake user_id, bool with_color = false) const;
    Snowflake GetMemberHighestRole(Snowflake guild_id, Snowflake user_id) const;
    std::unordered_set<Snowflake> GetUsersInGuild(Snowflake id) const;
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
    void UpdateStatus(PresenceStatus status, bool is_afk);
    void UpdateStatus(PresenceStatus status, bool is_afk, const ActivityData &obj);
    void CreateDM(Snowflake user_id);
    std::optional<Snowflake> FindDM(Snowflake user_id); // wont find group dms
    void AddReaction(Snowflake id, Glib::ustring param);
    void RemoveReaction(Snowflake id, Glib::ustring param);
    void SetGuildName(Snowflake id, const Glib::ustring &name);
    void SetGuildName(Snowflake id, const Glib::ustring &name, sigc::slot<void(bool success)> callback);
    void SetGuildIcon(Snowflake id, const std::string &data);
    void SetGuildIcon(Snowflake id, const std::string &data, sigc::slot<void(bool success)> callback);
    void UnbanUser(Snowflake guild_id, Snowflake user_id);
    void UnbanUser(Snowflake guild_id, Snowflake user_id, sigc::slot<void(bool success)> callback);
    void DeleteInvite(const std::string &code);
    void DeleteInvite(const std::string &code, sigc::slot<void(bool success)> callback);

    // FetchGuildBans fetches all bans+reasons via api, this func fetches stored bans (so usually just GUILD_BAN_ADD data)
    std::vector<BanData> GetBansInGuild(Snowflake guild_id);
    void FetchGuildBan(Snowflake guild_id, Snowflake user_id, sigc::slot<void(BanData)> callback);
    void FetchGuildBans(Snowflake guild_id, sigc::slot<void(std::vector<BanData>)> callback);

    void FetchInvite(std::string code, sigc::slot<void(std::optional<InviteData>)> callback);
    void FetchGuildInvites(Snowflake guild_id, sigc::slot<void(std::vector<InviteData>)> callback);

    void FetchAuditLog(Snowflake guild_id, sigc::slot<void(AuditLogData)> callback);

    void FetchUserProfile(Snowflake user_id, sigc::slot<void(UserProfileData)> callback);
    void FetchUserNote(Snowflake user_id, sigc::slot<void(std::string note)> callback);
    void SetUserNote(Snowflake user_id, std::string note);
    void SetUserNote(Snowflake user_id, std::string note, sigc::slot<void(bool success)> callback);
    void FetchUserRelationships(Snowflake user_id, sigc::slot<void(std::vector<UserData>)> callback);

    void UpdateToken(std::string token);
    void SetUserAgent(std::string agent);

    std::optional<PresenceStatus> GetUserStatus(Snowflake id) const;

private:
    static const constexpr int InflateChunkSize = 0x10000;
    std::vector<uint8_t> m_compressed_buf;
    std::vector<uint8_t> m_decompress_buf;
    z_stream m_zstream;

    void ProcessNewGuild(GuildData &guild);

    void HandleGatewayMessageRaw(std::string str);
    void HandleGatewayMessage(std::string str);
    void HandleGatewayHello(const GatewayMessage &msg);
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
    void HandleGatewayGuildRoleUpdate(const GatewayMessage &msg);
    void HandleGatewayGuildRoleCreate(const GatewayMessage &msg);
    void HandleGatewayGuildRoleDelete(const GatewayMessage &msg);
    void HandleGatewayMessageReactionAdd(const GatewayMessage &msg);
    void HandleGatewayMessageReactionRemove(const GatewayMessage &msg);
    void HandleGatewayChannelRecipientAdd(const GatewayMessage &msg);
    void HandleGatewayChannelRecipientRemove(const GatewayMessage &msg);
    void HandleGatewayTypingStart(const GatewayMessage &msg);
    void HandleGatewayGuildBanRemove(const GatewayMessage &msg);
    void HandleGatewayGuildBanAdd(const GatewayMessage &msg);
    void HandleGatewayInviteCreate(const GatewayMessage &msg);
    void HandleGatewayInviteDelete(const GatewayMessage &msg);
    void HandleGatewayUserNoteUpdate(const GatewayMessage &msg);
    void HandleGatewayReadySupplemental(const GatewayMessage &msg);
    void HandleGatewayReconnect(const GatewayMessage &msg);
    void HandleGatewayInvalidSession(const GatewayMessage &msg);
    void HeartbeatThread();
    void SendIdentify();
    void SendResume();

    void HandleSocketOpen();
    void HandleSocketClose(uint16_t code);

    bool CheckCode(const http::response_type &r);

    void StoreMessageData(Message &msg);

    std::string m_token;

    void AddMessageToChannel(Snowflake msg_id, Snowflake channel_id);
    std::unordered_map<Snowflake, std::unordered_set<Snowflake>> m_chan_to_message_map;

    void AddUserToGuild(Snowflake user_id, Snowflake guild_id);
    std::unordered_map<Snowflake, std::unordered_set<Snowflake>> m_guild_to_users;

    std::unordered_map<Snowflake, std::unordered_set<Snowflake>> m_guild_to_channels;

    std::unordered_map<Snowflake, PresenceStatus> m_user_to_status;

    UserData m_user_data;
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

    bool m_wants_resume = false;
    std::string m_session_id;

    mutable std::mutex m_msg_mutex;
    Glib::Dispatcher m_msg_dispatch;
    std::queue<std::string> m_msg_queue;
    void MessageDispatch();

    mutable std::mutex m_generic_mutex;
    Glib::Dispatcher m_generic_dispatch;
    std::queue<std::function<void()>> m_generic_queue;

    // signals
public:
    typedef sigc::signal<void> type_signal_gateway_ready;
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
    typedef sigc::signal<void, Snowflake> type_signal_role_update;
    typedef sigc::signal<void, Snowflake> type_signal_role_create;
    typedef sigc::signal<void, Snowflake> type_signal_role_delete;
    typedef sigc::signal<void, Snowflake, Glib::ustring> type_signal_reaction_add;
    typedef sigc::signal<void, Snowflake, Glib::ustring> type_signal_reaction_remove;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_typing_start;        // user id, channel id
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_guild_member_update; // guild id, user id
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_guild_ban_remove;    // guild id, user id
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_guild_ban_add;       // guild id, user id
    typedef sigc::signal<void, InviteData> type_signal_invite_create;
    typedef sigc::signal<void, InviteDeleteObject> type_signal_invite_delete;
    typedef sigc::signal<void, Snowflake, PresenceStatus> type_signal_presence_update;
    typedef sigc::signal<void, Snowflake, std::string> type_signal_note_update;
    typedef sigc::signal<void, bool, GatewayCloseCode> type_signal_disconnected; // bool true if reconnecting
    typedef sigc::signal<void> type_signal_connected;

    type_signal_gateway_ready signal_gateway_ready();
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
    type_signal_role_update signal_role_update();
    type_signal_role_create signal_role_create();
    type_signal_role_delete signal_role_delete();
    type_signal_reaction_add signal_reaction_add();
    type_signal_reaction_remove signal_reaction_remove();
    type_signal_typing_start signal_typing_start();
    type_signal_guild_member_update signal_guild_member_update();
    type_signal_guild_ban_remove signal_guild_ban_remove();
    type_signal_guild_ban_add signal_guild_ban_add();
    type_signal_invite_create signal_invite_create();
    type_signal_invite_delete signal_invite_delete(); // safe to assume guild id is set
    type_signal_presence_update signal_presence_update();
    type_signal_note_update signal_note_update();
    type_signal_disconnected signal_disconnected();
    type_signal_connected signal_connected();

protected:
    type_signal_gateway_ready m_signal_gateway_ready;
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
    type_signal_role_update m_signal_role_update;
    type_signal_role_create m_signal_role_create;
    type_signal_role_delete m_signal_role_delete;
    type_signal_reaction_add m_signal_reaction_add;
    type_signal_reaction_remove m_signal_reaction_remove;
    type_signal_typing_start m_signal_typing_start;
    type_signal_guild_member_update m_signal_guild_member_update;
    type_signal_guild_ban_remove m_signal_guild_ban_remove;
    type_signal_guild_ban_add m_signal_guild_ban_add;
    type_signal_invite_create m_signal_invite_create;
    type_signal_invite_delete m_signal_invite_delete;
    type_signal_presence_update m_signal_presence_update;
    type_signal_note_update m_signal_note_update;
    type_signal_disconnected m_signal_disconnected;
    type_signal_connected m_signal_connected;
};
