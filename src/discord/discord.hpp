#pragma once
#include "chatsubmitparams.hpp"
#include "waiter.hpp"
#include "httpclient.hpp"
#include "objects.hpp"
#include "store.hpp"
#include "voiceclient.hpp"
#include "voicestateflags.hpp"
#include "websocket.hpp"
#include <gdkmm/rgba.h>
#include <sigc++/sigc++.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <map>
#include <set>
#include <mutex>
#include <zlib.h>
#include <glibmm.h>
#include <queue>

#ifdef GetMessage
#undef GetMessage
#endif

class DiscordClient {
    friend class Abaddon;

public:
    DiscordClient(bool mem_store = false);
    void Start();
    bool Stop();
    bool IsStarted() const;
    bool IsStoreValid() const;

    std::unordered_set<Snowflake> GetGuilds() const;
    const UserData &GetUserData() const;
    const UserGuildSettingsData &GetUserGuildSettings() const;
    std::optional<UserGuildSettingsEntry> GetSettingsForGuild(Snowflake id) const;
    std::vector<Snowflake> GetUserSortedGuilds() const;
    std::vector<Message> GetMessagesForChannel(Snowflake id, size_t limit = 50) const;
    std::vector<Message> GetMessagesBefore(Snowflake channel_id, Snowflake message_id, size_t limit = 50) const;
    std::set<Snowflake> GetPrivateChannels() const;
    const UserSettings &GetUserSettings() const;

    EPremiumType GetSelfPremiumType() const;

    void FetchMessagesInChannel(Snowflake id, const sigc::slot<void(const std::vector<Message> &)> &cb);
    void FetchMessagesInChannelBefore(Snowflake channel_id, Snowflake before_id, const sigc::slot<void(const std::vector<Message> &)> &cb);
    std::optional<Message> GetMessage(Snowflake id) const;
    std::optional<ChannelData> GetChannel(Snowflake id) const;
    std::optional<EmojiData> GetEmoji(Snowflake id) const;
    std::optional<PermissionOverwrite> GetPermissionOverwrite(Snowflake channel_id, Snowflake id) const;
    std::optional<UserData> GetUser(Snowflake id) const;
    std::optional<RoleData> GetRole(Snowflake id) const;
    std::optional<GuildData> GetGuild(Snowflake id) const;
    std::optional<GuildMember> GetMember(Snowflake user_id, Snowflake guild_id) const;
    Snowflake GetMemberHoistedRole(Snowflake guild_id, Snowflake user_id, bool with_color = false) const;
    std::optional<RoleData> GetMemberHoistedRoleCached(const GuildMember &member, const std::unordered_map<Snowflake, RoleData> &roles, bool with_color = false) const;
    std::optional<RoleData> GetMemberHighestRole(Snowflake guild_id, Snowflake user_id) const;
    std::set<Snowflake> GetUsersInGuild(Snowflake id) const;
    std::set<Snowflake> GetChannelsInGuild(Snowflake id) const;
    std::vector<Snowflake> GetUsersInThread(Snowflake id) const;
    std::vector<ChannelData> GetActiveThreads(Snowflake channel_id) const;
    void GetArchivedPublicThreads(Snowflake channel_id, const sigc::slot<void(DiscordError, const ArchivedThreadsResponseData &)> &callback);
    void GetArchivedPrivateThreads(Snowflake channel_id, const sigc::slot<void(DiscordError, const ArchivedThreadsResponseData &)> &callback);
    std::vector<Snowflake> GetChildChannelIDs(Snowflake parent_id) const;
    std::optional<WebhookMessageData> GetWebhookMessageData(Snowflake message_id) const;

    // get ids of given list of members for who we do not have the member data
    template<typename Iter>
    std::unordered_set<Snowflake> FilterUnknownMembersFrom(Snowflake guild_id, Iter begin, Iter end) {
        std::unordered_set<Snowflake> ret;
        const auto known = m_store.GetMembersInGuild(guild_id);
        for (auto iter = begin; iter != end; iter++)
            if (known.find(*iter) == known.end())
                ret.insert(*iter);
        return ret;
    }

    bool IsThreadJoined(Snowflake thread_id) const;
    bool HasGuildPermission(Snowflake user_id, Snowflake guild_id, Permission perm) const;

    bool HasSelfChannelPermission(Snowflake channel_id, Permission perm) const;
    bool HasAnyChannelPermission(Snowflake user_id, Snowflake channel_id, Permission perm) const;
    bool HasChannelPermission(Snowflake user_id, Snowflake channel_id, Permission perm) const;
    Permission ComputePermissions(Snowflake member_id, Snowflake guild_id) const;
    Permission ComputeOverwrites(Permission base, Snowflake member_id, Snowflake channel_id) const;
    bool CanManageMember(Snowflake guild_id, Snowflake actor, Snowflake target) const; // kick, ban, edit nickname (cant think of a better name)

    void ChatMessageCallback(const std::string &nonce, const http::response_type &response, const sigc::slot<void(DiscordError code)> &callback);
    void SendChatMessageNoAttachments(const ChatSubmitParams &params, const sigc::slot<void(DiscordError code)> &callback);
    void SendChatMessageAttachments(const ChatSubmitParams &params, const sigc::slot<void(DiscordError code)> &callback);

    void SendChatMessage(const ChatSubmitParams &params, const sigc::slot<void(DiscordError code)> &callback);
    void DeleteMessage(Snowflake channel_id, Snowflake id);
    void EditMessage(Snowflake channel_id, Snowflake id, std::string content);
    void SendLazyLoad(Snowflake id);
    void SendThreadLazyLoad(Snowflake id);
    void LeaveGuild(Snowflake id);
    void KickUser(Snowflake user_id, Snowflake guild_id);
    void BanUser(Snowflake user_id, Snowflake guild_id); // todo: reason, delete messages
    void UpdateStatus(PresenceStatus status, bool is_afk);
    void UpdateStatus(PresenceStatus status, bool is_afk, const ActivityData &obj);
    void CloseDM(Snowflake channel_id);
    std::optional<Snowflake> FindDM(Snowflake user_id); // wont find group dms
    void AddReaction(Snowflake id, Glib::ustring param);
    void RemoveReaction(Snowflake id, Glib::ustring param);
    void SetGuildName(Snowflake id, const Glib::ustring &name, const sigc::slot<void(DiscordError code)> &callback);
    void SetGuildIcon(Snowflake id, const std::string &data, const sigc::slot<void(DiscordError code)> &callback);
    void UnbanUser(Snowflake guild_id, Snowflake user_id, const sigc::slot<void(DiscordError code)> &callback);
    void DeleteInvite(const std::string &code, const sigc::slot<void(DiscordError code)> &callback);
    void AddGroupDMRecipient(Snowflake channel_id, Snowflake user_id);
    void RemoveGroupDMRecipient(Snowflake channel_id, Snowflake user_id);
    void ModifyRolePermissions(Snowflake guild_id, Snowflake role_id, Permission permissions, const sigc::slot<void(DiscordError code)> &callback);
    void ModifyRoleName(Snowflake guild_id, Snowflake role_id, const Glib::ustring &name, const sigc::slot<void(DiscordError code)> &callback);
    void ModifyRoleColor(Snowflake guild_id, Snowflake role_id, uint32_t color, const sigc::slot<void(DiscordError code)> &callback);
    void ModifyRoleColor(Snowflake guild_id, Snowflake role_id, const Gdk::RGBA &color, const sigc::slot<void(DiscordError code)> &callback);
    void ModifyRolePosition(Snowflake guild_id, Snowflake role_id, int position, const sigc::slot<void(DiscordError code)> &callback);
    void ModifyEmojiName(Snowflake guild_id, Snowflake emoji_id, const Glib::ustring &name, const sigc::slot<void(DiscordError code)> &callback);
    void DeleteEmoji(Snowflake guild_id, Snowflake emoji_id, const sigc::slot<void(DiscordError code)> &callback);
    void RemoveRelationship(Snowflake id, const sigc::slot<void(DiscordError code)> &callback);
    void SendFriendRequest(const Glib::ustring &username, int discriminator, const sigc::slot<void(DiscordError code)> &callback);
    void PutRelationship(Snowflake id, const sigc::slot<void(DiscordError code)> &callback); // send fr by id, accept incoming
    void Pin(Snowflake channel_id, Snowflake message_id, const sigc::slot<void(DiscordError code)> &callback);
    void Unpin(Snowflake channel_id, Snowflake message_id, const sigc::slot<void(DiscordError code)> &callback);
    void LeaveThread(Snowflake channel_id, const std::string &location, const sigc::slot<void(DiscordError code)> &callback);
    void ArchiveThread(Snowflake channel_id, const sigc::slot<void(DiscordError code)> &callback);
    void UnArchiveThread(Snowflake channel_id, const sigc::slot<void(DiscordError code)> &callback);
    void MarkChannelAsRead(Snowflake channel_id, const sigc::slot<void(DiscordError code)> &callback);
    void MarkGuildAsRead(Snowflake guild_id, const sigc::slot<void(DiscordError code)> &callback);
    void MuteChannel(Snowflake channel_id, const sigc::slot<void(DiscordError code)> &callback);
    void UnmuteChannel(Snowflake channel_id, const sigc::slot<void(DiscordError code)> &callback);
    void MuteGuild(Snowflake id, const sigc::slot<void(DiscordError code)> &callback);
    void UnmuteGuild(Snowflake id, const sigc::slot<void(DiscordError code)> &callback);
    void MuteThread(Snowflake id, const sigc::slot<void(DiscordError code)> &callback);
    void UnmuteThread(Snowflake id, const sigc::slot<void(DiscordError code)> &callback);

    bool CanModifyRole(Snowflake guild_id, Snowflake role_id) const;
    bool CanModifyRole(Snowflake guild_id, Snowflake role_id, Snowflake user_id) const;

    // send op 8 to get member data for unknown members
    template<typename Iter>
    void RequestMembers(Snowflake guild_id, Iter begin, Iter end) {
        if (std::distance(begin, end) == 0) return;

        RequestGuildMembersMessage obj;
        obj.GuildID = guild_id;
        obj.Presences = false;
        obj.UserIDs = { begin, end };
        m_websocket.Send(obj);
    }

    // real client doesn't seem to use the single role endpoints so neither do we
    template<typename Iter>
    auto SetMemberRoles(Snowflake guild_id, Snowflake user_id, Iter begin, Iter end, const sigc::slot<void(DiscordError code)> &callback) {
        ModifyGuildMemberObject obj;
        obj.Roles = { begin, end };
        m_http.MakePATCH("/guilds/" + std::to_string(guild_id) + "/members/" + std::to_string(user_id), nlohmann::json(obj).dump(), [this, callback](const http::response_type &response) {
            if (CheckCode(response))
                callback(DiscordError::NONE);
            else
                callback(GetCodeFromResponse(response));
        });
    }

    template<typename Iter>
    std::vector<UserData> GetUsersBulk(Iter begin, Iter end) {
        return m_store.GetUsersBulk(begin, end);
    }

    // FetchGuildBans fetches all bans+reasons via api, this func fetches stored bans (so usually just GUILD_BAN_ADD data)
    std::vector<BanData> GetBansInGuild(Snowflake guild_id);
    void FetchGuildBan(Snowflake guild_id, Snowflake user_id, const sigc::slot<void(BanData)> &callback);
    void FetchGuildBans(Snowflake guild_id, const sigc::slot<void(std::vector<BanData>)> &callback);

    void FetchInvite(const std::string &code, const sigc::slot<void(std::optional<InviteData>)> &callback);
    void FetchGuildInvites(Snowflake guild_id, const sigc::slot<void(std::vector<InviteData>)> &callback);

    void FetchAuditLog(Snowflake guild_id, const sigc::slot<void(AuditLogData)> &callback);

    void FetchGuildEmojis(Snowflake guild_id, const sigc::slot<void(std::vector<EmojiData>)> &callback);

    void FetchUserProfile(Snowflake user_id, const sigc::slot<void(UserProfileData)> &callback);
    void FetchUserNote(Snowflake user_id, const sigc::slot<void(std::string note)> &callback);
    void SetUserNote(Snowflake user_id, std::string note, const sigc::slot<void(DiscordError code)> &callback);
    void FetchUserRelationships(Snowflake user_id, const sigc::slot<void(std::vector<UserData>)> &callback);

    void FetchPinned(Snowflake id, const sigc::slot<void(std::vector<Message>, DiscordError code)> &callback);

    bool IsVerificationRequired(Snowflake guild_id) const;
    void GetVerificationGateInfo(Snowflake guild_id, const sigc::slot<void(std::optional<VerificationGateInfoObject>)> &callback);
    void AcceptVerificationGate(Snowflake guild_id, VerificationGateInfoObject info, const sigc::slot<void(DiscordError code)> &callback);

    void RemoteAuthLogin(const std::string &ticket, const sigc::slot<void(std::optional<std::string>, DiscordError code)> &callback);

#ifdef WITH_VOICE
    void ConnectToVoice(Snowflake channel_id);
    void DisconnectFromVoice();
    // Is fully connected
    [[nodiscard]] bool IsVoiceConnected() const noexcept;
    [[nodiscard]] bool IsVoiceConnecting() const noexcept;
    [[nodiscard]] Snowflake GetVoiceChannelID() const noexcept;
    [[nodiscard]] std::unordered_set<Snowflake> GetUsersInVoiceChannel(Snowflake channel_id);
    [[nodiscard]] std::optional<uint32_t> GetSSRCOfUser(Snowflake id) const;
    [[nodiscard]] std::optional<std::pair<Snowflake, VoiceStateFlags>> GetVoiceState(Snowflake user_id) const;

    DiscordVoiceClient &GetVoiceClient();

    void SetVoiceMuted(bool is_mute);
    void SetVoiceDeafened(bool is_deaf);
#endif

    void SetReferringChannel(Snowflake id);

    void SetBuildNumber(uint32_t build_number);
    void SetCookie(std::string_view cookie);

    void UpdateToken(const std::string &token);
    void SetUserAgent(const std::string &agent);

    void SetDumpReady(bool dump);

    bool IsChannelMuted(Snowflake id) const noexcept;
    bool IsGuildMuted(Snowflake id) const noexcept;
    int GetUnreadStateForChannel(Snowflake id) const noexcept;
    int GetUnreadChannelsCountForCategory(Snowflake id) const noexcept;
    bool GetUnreadStateForGuild(Snowflake id, int &total_mentions) const noexcept;
    int GetUnreadDMsCount() const;

    PresenceStatus GetUserStatus(Snowflake id) const;

    std::map<Snowflake, RelationshipType> GetRelationships() const;
    std::set<Snowflake> GetRelationships(RelationshipType type) const;
    std::optional<RelationshipType> GetRelationship(Snowflake id) const;

private:
    static const constexpr int InflateChunkSize = 0x10000;
    std::vector<uint8_t> m_compressed_buf;
    std::vector<uint8_t> m_decompress_buf;
    z_stream m_zstream;

    bool m_dump_ready = false;

    static std::string GetAPIURL();
    static std::string GetGatewayURL();

    static DiscordError GetCodeFromResponse(const http::response_type &response);

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
    void HandleGatewayGuildEmojisUpdate(const GatewayMessage &msg);
    void HandleGatewayGuildJoinRequestCreate(const GatewayMessage &msg);
    void HandleGatewayGuildJoinRequestUpdate(const GatewayMessage &msg);
    void HandleGatewayGuildJoinRequestDelete(const GatewayMessage &msg);
    void HandleGatewayRelationshipRemove(const GatewayMessage &msg);
    void HandleGatewayRelationshipAdd(const GatewayMessage &msg);
    void HandleGatewayThreadCreate(const GatewayMessage &msg);
    void HandleGatewayThreadDelete(const GatewayMessage &msg);
    void HandleGatewayThreadListSync(const GatewayMessage &msg);
    void HandleGatewayThreadMembersUpdate(const GatewayMessage &msg);
    void HandleGatewayThreadMemberUpdate(const GatewayMessage &msg);
    void HandleGatewayThreadUpdate(const GatewayMessage &msg);
    void HandleGatewayThreadMemberListUpdate(const GatewayMessage &msg);
    void HandleGatewayMessageAck(const GatewayMessage &msg);
    void HandleGatewayUserGuildSettingsUpdate(const GatewayMessage &msg);
    void HandleGatewayGuildMembersChunk(const GatewayMessage &msg);
    void HandleGatewayReadySupplemental(const GatewayMessage &msg);
    void HandleGatewayReconnect(const GatewayMessage &msg);
    void HandleGatewayInvalidSession(const GatewayMessage &msg);

#ifdef WITH_VOICE
    void HandleGatewayVoiceStateUpdate(const GatewayMessage &msg);
    void HandleGatewayVoiceServerUpdate(const GatewayMessage &msg);
    void HandleGatewayCallCreate(const GatewayMessage &msg);

    void CheckVoiceState(const VoiceState &data);
#endif

    void HeartbeatThread();
    void SendIdentify();
    void SendResume();

    void SetHeaders();
    void SetSuperPropertiesFromIdentity(const IdentifyMessage &identity);

    void HandleSocketOpen();
    void HandleSocketClose(const ix::WebSocketCloseInfo &info);

    static bool CheckCode(const http::response_type &r);
    static bool CheckCode(const http::response_type &r, int expected);

    void StoreMessageData(Message &msg);

    void HandleReadyReadState(const ReadyEventData &data);
    void HandleReadyGuildSettings(const ReadyEventData &data);

    std::string m_token;

    uint32_t m_build_number = 279382;

    void AddUserToGuild(Snowflake user_id, Snowflake guild_id);
    std::map<Snowflake, std::set<Snowflake>> m_guild_to_users;
    std::map<Snowflake, std::set<Snowflake>> m_guild_to_channels;
    std::map<Snowflake, GuildApplicationData> m_guild_join_requests;
    std::map<Snowflake, PresenceStatus> m_user_to_status;
    std::map<Snowflake, RelationshipType> m_user_relationships;
    std::set<Snowflake> m_joined_threads;
    std::map<Snowflake, std::vector<Snowflake>> m_thread_members;
    std::map<Snowflake, Snowflake> m_last_message_id;
    std::unordered_set<Snowflake> m_muted_guilds;
    std::unordered_set<Snowflake> m_muted_channels;
    std::unordered_map<Snowflake, int> m_unread;
    std::unordered_set<Snowflake> m_channel_muted_parent;

    UserData m_user_data;
    UserSettings m_user_settings;
    UserGuildSettingsData m_user_guild_settings;

    Store m_store;
    HTTPClient m_http;
    Websocket m_websocket;
    std::atomic<bool> m_client_connected = false;
    std::atomic<bool> m_ready_received = false;
    bool m_client_started = false;

    std::map<std::string, GatewayEvent> m_event_map;
    void LoadEventMap();

    std::thread m_heartbeat_thread;
    std::atomic<int> m_last_sequence = -1;
    std::atomic<int> m_heartbeat_msec = 0;
    Waiter m_heartbeat_waiter;
    std::atomic<bool> m_heartbeat_acked = true;

    bool m_reconnecting = false; // reconnecting either to resume or reidentify
    bool m_wants_resume = false; // reconnecting specifically to resume
    std::string m_session_id;

#ifdef WITH_VOICE
    DiscordVoiceClient m_voice;

    bool m_mute_requested = false;
    bool m_deaf_requested = false;

    Snowflake m_voice_channel_id;
    // todo sql i guess
    std::unordered_map<Snowflake, std::pair<Snowflake, VoiceStateFlags>> m_voice_states;
    std::unordered_map<Snowflake, std::unordered_set<Snowflake>> m_voice_state_channel_users;

    void SendVoiceStateUpdate();

    void SetVoiceState(Snowflake user_id, const VoiceState &state);
    void ClearVoiceState(Snowflake user_id);

    void OnVoiceConnected();
    void OnVoiceDisconnected();
#endif

    mutable std::mutex m_msg_mutex;
    Glib::Dispatcher m_msg_dispatch;
    std::queue<std::string> m_msg_queue;
    void MessageDispatch();

    mutable std::mutex m_generic_mutex;
    Glib::Dispatcher m_generic_dispatch;
    std::queue<std::function<void()>> m_generic_queue;

    Glib::Timer m_progress_cb_timer;

    std::set<Snowflake> m_channels_pinned_requested;
    std::set<Snowflake> m_channels_lazy_loaded;

    // signals
public:
    typedef sigc::signal<void> type_signal_gateway_ready;
    typedef sigc::signal<void> type_signal_gateway_ready_supplemental;
    typedef sigc::signal<void, Message> type_signal_message_create;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_message_delete;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_message_update;
    typedef sigc::signal<void, Snowflake> type_signal_guild_member_list_update;
    typedef sigc::signal<void, GuildData> type_signal_guild_create;
    typedef sigc::signal<void, Snowflake> type_signal_guild_delete;
    typedef sigc::signal<void, Snowflake> type_signal_channel_delete;
    typedef sigc::signal<void, Snowflake> type_signal_channel_update;
    typedef sigc::signal<void, ChannelData> type_signal_channel_create;
    typedef sigc::signal<void, Snowflake> type_signal_guild_update;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_role_update; // guild id, role id
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_role_create; // guild id, role id
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_role_delete; // guild id, role id
    typedef sigc::signal<void, Snowflake, Glib::ustring> type_signal_reaction_add;
    typedef sigc::signal<void, Snowflake, Glib::ustring> type_signal_reaction_remove;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_typing_start;        // user id, channel id
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_guild_member_update; // guild id, user id
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_guild_ban_remove;    // guild id, user id
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_guild_ban_add;       // guild id, user id
    typedef sigc::signal<void, InviteData> type_signal_invite_create;
    typedef sigc::signal<void, InviteDeleteObject> type_signal_invite_delete;
    typedef sigc::signal<void, UserData, PresenceStatus> type_signal_presence_update;
    typedef sigc::signal<void, Snowflake, std::string> type_signal_note_update;
    typedef sigc::signal<void, Snowflake, std::vector<EmojiData>> type_signal_guild_emojis_update; // guild id
    typedef sigc::signal<void, GuildJoinRequestCreateData> type_signal_guild_join_request_create;
    typedef sigc::signal<void, GuildJoinRequestUpdateData> type_signal_guild_join_request_update;
    typedef sigc::signal<void, GuildJoinRequestDeleteData> type_signal_guild_join_request_delete;
    typedef sigc::signal<void, Snowflake, RelationshipType> type_signal_relationship_remove;
    typedef sigc::signal<void, RelationshipAddData> type_signal_relationship_add;
    typedef sigc::signal<void, ChannelData> type_signal_thread_create;
    typedef sigc::signal<void, ThreadDeleteData> type_signal_thread_delete;
    typedef sigc::signal<void, ThreadListSyncData> type_signal_thread_list_sync;
    typedef sigc::signal<void, ThreadMembersUpdateData> type_signal_thread_members_update;
    typedef sigc::signal<void, ThreadUpdateData> type_signal_thread_update;
    typedef sigc::signal<void, ThreadMemberListUpdateData> type_signal_thread_member_list_update;
    typedef sigc::signal<void, MessageAckData> type_signal_message_ack;
    typedef sigc::signal<void, GuildMembersChunkData> type_signal_guild_members_chunk;

    // not discord dispatch events
    typedef sigc::signal<void, Snowflake> type_signal_added_to_thread;
    typedef sigc::signal<void, Snowflake> type_signal_removed_from_thread;
    typedef sigc::signal<void, Message> type_signal_message_unpinned;
    typedef sigc::signal<void, Message> type_signal_message_pinned;
    typedef sigc::signal<void, Message> type_signal_message_sent;
    typedef sigc::signal<void, Snowflake> type_signal_channel_muted;
    typedef sigc::signal<void, Snowflake> type_signal_channel_unmuted;
    typedef sigc::signal<void, Snowflake> type_signal_guild_muted;
    typedef sigc::signal<void, Snowflake> type_signal_guild_unmuted;
    typedef sigc::signal<void, Snowflake, bool> type_signal_channel_accessibility_changed;

    typedef sigc::signal<void, std::string /* nonce */, float /* retry_after */> type_signal_message_send_fail; // retry after param will be 0 if it failed for a reason that isnt slowmode
    typedef sigc::signal<void, bool, GatewayCloseCode> type_signal_disconnected;                                // bool true if reconnecting
    typedef sigc::signal<void> type_signal_connected;
    typedef sigc::signal<void, std::string, float> type_signal_message_progress;

#ifdef WITH_VOICE
    using type_signal_voice_connected = sigc::signal<void()>;
    using type_signal_voice_disconnected = sigc::signal<void()>;
    using type_signal_voice_speaking = sigc::signal<void(VoiceSpeakingData)>;
    using type_signal_voice_user_disconnect = sigc::signal<void(Snowflake, Snowflake)>;
    using type_signal_voice_user_connect = sigc::signal<void(Snowflake, Snowflake)>;
    using type_signal_voice_requested_connect = sigc::signal<void(Snowflake)>;
    using type_signal_voice_requested_disconnect = sigc::signal<void()>;
    using type_signal_voice_client_state_update = sigc::signal<void(DiscordVoiceClient::State)>;
    using type_signal_voice_channel_changed = sigc::signal<void(Snowflake)>;
    using type_signal_voice_state_set = sigc::signal<void(Snowflake, Snowflake, VoiceStateFlags)>;
#endif

    type_signal_gateway_ready signal_gateway_ready();
    type_signal_gateway_ready_supplemental signal_gateway_ready_supplemental();
    type_signal_message_create signal_message_create();
    type_signal_message_delete signal_message_delete();
    type_signal_message_update signal_message_update();
    type_signal_guild_member_list_update signal_guild_member_list_update();
    type_signal_guild_create signal_guild_create(); // structs are complete in this signal
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
    type_signal_guild_emojis_update signal_guild_emojis_update();
    type_signal_guild_join_request_create signal_guild_join_request_create();
    type_signal_guild_join_request_update signal_guild_join_request_update();
    type_signal_guild_join_request_delete signal_guild_join_request_delete();
    type_signal_relationship_remove signal_relationship_remove();
    type_signal_relationship_add signal_relationship_add();
    type_signal_message_unpinned signal_message_unpinned();
    type_signal_message_pinned signal_message_pinned();
    type_signal_thread_create signal_thread_create();
    type_signal_thread_delete signal_thread_delete();
    type_signal_thread_list_sync signal_thread_list_sync();
    type_signal_thread_members_update signal_thread_members_update();
    type_signal_thread_update signal_thread_update();
    type_signal_thread_member_list_update signal_thread_member_list_update();
    type_signal_message_ack signal_message_ack();
    type_signal_guild_members_chunk signal_guild_members_chunk();

    type_signal_added_to_thread signal_added_to_thread();
    type_signal_removed_from_thread signal_removed_from_thread();
    type_signal_message_sent signal_message_sent();
    type_signal_channel_muted signal_channel_muted();
    type_signal_channel_unmuted signal_channel_unmuted();
    type_signal_guild_muted signal_guild_muted();
    type_signal_guild_unmuted signal_guild_unmuted();
    type_signal_channel_accessibility_changed signal_channel_accessibility_changed();
    type_signal_message_send_fail signal_message_send_fail();
    type_signal_disconnected signal_disconnected();
    type_signal_connected signal_connected();
    type_signal_message_progress signal_message_progress();

#ifdef WITH_VOICE
    type_signal_voice_connected signal_voice_connected();
    type_signal_voice_disconnected signal_voice_disconnected();
    type_signal_voice_speaking signal_voice_speaking();
    type_signal_voice_user_disconnect signal_voice_user_disconnect();
    type_signal_voice_user_connect signal_voice_user_connect();
    type_signal_voice_requested_connect signal_voice_requested_connect();
    type_signal_voice_requested_disconnect signal_voice_requested_disconnect();
    type_signal_voice_client_state_update signal_voice_client_state_update();
    type_signal_voice_channel_changed signal_voice_channel_changed();
    type_signal_voice_state_set signal_voice_state_set();
#endif

protected:
    type_signal_gateway_ready m_signal_gateway_ready;
    type_signal_gateway_ready_supplemental m_signal_gateway_ready_supplemental;
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
    type_signal_guild_emojis_update m_signal_guild_emojis_update;
    type_signal_guild_join_request_create m_signal_guild_join_request_create;
    type_signal_guild_join_request_update m_signal_guild_join_request_update;
    type_signal_guild_join_request_delete m_signal_guild_join_request_delete;
    type_signal_relationship_remove m_signal_relationship_remove;
    type_signal_relationship_add m_signal_relationship_add;
    type_signal_message_unpinned m_signal_message_unpinned;
    type_signal_message_pinned m_signal_message_pinned;
    type_signal_thread_create m_signal_thread_create;
    type_signal_thread_delete m_signal_thread_delete;
    type_signal_thread_list_sync m_signal_thread_list_sync;
    type_signal_thread_members_update m_signal_thread_members_update;
    type_signal_thread_update m_signal_thread_update;
    type_signal_thread_member_list_update m_signal_thread_member_list_update;
    type_signal_message_ack m_signal_message_ack;
    type_signal_guild_members_chunk m_signal_guild_members_chunk;

    type_signal_removed_from_thread m_signal_removed_from_thread;
    type_signal_added_to_thread m_signal_added_to_thread;
    type_signal_message_sent m_signal_message_sent;
    type_signal_channel_muted m_signal_channel_muted;
    type_signal_channel_unmuted m_signal_channel_unmuted;
    type_signal_guild_muted m_signal_guild_muted;
    type_signal_guild_unmuted m_signal_guild_unmuted;
    type_signal_channel_accessibility_changed m_signal_channel_accessibility_changed;
    type_signal_message_send_fail m_signal_message_send_fail;
    type_signal_disconnected m_signal_disconnected;
    type_signal_connected m_signal_connected;
    type_signal_message_progress m_signal_message_progress;

#ifdef WITH_VOICE
    type_signal_voice_connected m_signal_voice_connected;
    type_signal_voice_disconnected m_signal_voice_disconnected;
    type_signal_voice_speaking m_signal_voice_speaking;
    type_signal_voice_user_disconnect m_signal_voice_user_disconnect;
    type_signal_voice_user_connect m_signal_voice_user_connect;
    type_signal_voice_requested_connect m_signal_voice_requested_connect;
    type_signal_voice_requested_disconnect m_signal_voice_requested_disconnect;
    type_signal_voice_client_state_update m_signal_voice_client_state_update;
    type_signal_voice_channel_changed m_signal_voice_channel_changed;
    type_signal_voice_state_set m_signal_voice_state_set;
#endif
};
