#include "store.hpp"
#include <cinttypes>

using namespace std::literals::string_literals;

// hopefully the casting between signed and unsigned int64 doesnt cause issues

Store::Store(bool mem_store)
    : m_db_path(mem_store ? ":memory:" : std::filesystem::temp_directory_path() / "abaddon-store.db")
    , m_db(m_db_path.string().c_str()) {
    if (!m_db.OK()) {
        fprintf(stderr, "error opening database: %s\n", m_db.ErrStr());
        return;
    }

    if (m_db.Execute("PRAGMA journal_mode = WAL") != SQLITE_OK) {
        fprintf(stderr, "enabling write-ahead-log failed: %s\n", m_db.ErrStr());
        return;
    }

    if (m_db.Execute("PRAGMA synchronous = NORMAL") != SQLITE_OK) {
        fprintf(stderr, "setting synchronous failed: %s\n", m_db.ErrStr());
        return;
    }

    m_ok &= CreateTables();
    m_ok &= CreateStatements();
}

Store::~Store() {}

bool Store::IsValid() const {
    return m_db.OK() && m_ok;
}

void Store::SetBan(Snowflake guild_id, Snowflake user_id, const BanData &ban) {
    auto &s = m_stmt_set_ban;

    s->Bind(1, guild_id);
    s->Bind(2, user_id);
    s->Bind(3, ban.Reason);

    if (!s->Insert())
        fprintf(stderr, "ban insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(guild_id), static_cast<uint64_t>(user_id), m_db.ErrStr());

    s->Reset();
}

void Store::SetWebhookMessage(const Message &message) {
    auto &s = m_stmt_set_webhook_msg;

    s->Bind(1, message.ID);
    s->Bind(2, message.WebhookID);
    s->Bind(3, message.Author.Username);
    s->Bind(4, message.Author.Avatar);

    if (!s->Insert())
        fprintf(stderr, "webhook message insert failed for %" PRIu64 ": %s\n", static_cast<uint64_t>(message.ID), m_db.ErrStr());

    s->Reset();
}

void Store::SetChannel(Snowflake id, const ChannelData &chan) {
    auto &s = m_stmt_set_chan;

    s->Bind(1, id);
    s->Bind(2, chan.Type);
    s->Bind(3, chan.GuildID);
    s->Bind(4, chan.Position);
    s->Bind(5, chan.Name);
    s->Bind(6, chan.Topic);
    s->Bind(7, chan.IsNSFW);
    s->Bind(8, chan.LastMessageID);
    s->Bind(9, chan.Bitrate);
    s->Bind(10, chan.UserLimit);
    s->Bind(11, chan.RateLimitPerUser);
    s->Bind(12, chan.Icon);
    s->Bind(13, chan.OwnerID);
    s->Bind(14, chan.ApplicationID);
    s->Bind(15, chan.ParentID);
    s->Bind(16, chan.LastPinTimestamp);
    if (chan.ThreadMetadata.has_value()) {
        s->Bind(17, chan.ThreadMetadata->IsArchived);
        s->Bind(18, chan.ThreadMetadata->AutoArchiveDuration);
        s->Bind(19, chan.ThreadMetadata->ArchiveTimestamp);
    } else {
        s->Bind(17);
        s->Bind(18);
        s->Bind(19);
    }

    if (!s->Insert())
        fprintf(stderr, "channel insert failed for %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());

    if (chan.Recipients.has_value()) {
        BeginTransaction();
        auto &s = m_stmt_set_recipient;
        for (const auto &r : *chan.Recipients) {
            s->Bind(1, chan.ID);
            s->Bind(2, r.ID);
            if (!s->Insert())
                fprintf(stderr, "recipient insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(chan.ID), static_cast<uint64_t>(r.ID), m_db.ErrStr());
            s->Reset();
        }
        EndTransaction();
    } else if (chan.RecipientIDs.has_value()) {
        BeginTransaction();
        auto &s = m_stmt_set_recipient;
        for (const auto &id : *chan.RecipientIDs) {
            s->Bind(1, chan.ID);
            s->Bind(2, id);
            if (!s->Insert())
                fprintf(stderr, "recipient insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(chan.ID), static_cast<uint64_t>(id), m_db.ErrStr());
            s->Reset();
        }
        EndTransaction();
    }

    s->Reset();
}

void Store::SetEmoji(Snowflake id, const EmojiData &emoji) {
    auto &s = m_stmt_set_emoji;

    s->Bind(1, id);
    s->Bind(2, emoji.Name);
    if (emoji.Creator.has_value())
        s->Bind(3, emoji.Creator->ID);
    else
        s->Bind(3);
    s->Bind(4, emoji.NeedsColons);
    s->Bind(5, emoji.IsManaged);
    s->Bind(6, emoji.IsAnimated);
    s->Bind(7, emoji.IsAvailable);

    if (emoji.Roles.has_value()) {
        BeginTransaction();

        auto &s = m_stmt_set_emoji_role;

        for (const auto &r : *emoji.Roles) {
            s->Bind(1, id);
            s->Bind(2, r);
            if (!s->Insert())
                fprintf(stderr, "emoji role insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(id), static_cast<uint64_t>(r), m_db.ErrStr());
            s->Reset();
        }

        EndTransaction();
    }

    if (!s->Insert())
        fprintf(stderr, "emoji insert failed for %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());

    s->Reset();
}

void Store::SetGuild(Snowflake id, const GuildData &guild) {
    BeginTransaction();
    auto &s = m_stmt_set_guild;

    s->Bind(1, guild.ID);
    s->Bind(2, guild.Name);
    s->Bind(3, guild.Icon);
    s->Bind(4, guild.Splash);
    s->Bind(5, guild.IsOwner);
    s->Bind(6, guild.OwnerID);
    s->Bind(7, guild.PermissionsNew);
    s->Bind(8, guild.VoiceRegion);
    s->Bind(9, guild.AFKChannelID);
    s->Bind(10, guild.AFKTimeout);
    s->Bind(11, guild.VerificationLevel);
    s->Bind(12, guild.DefaultMessageNotifications);
    s->Bind(13, guild.MFALevel);
    s->Bind(14, guild.ApplicationID);
    s->Bind(15, guild.IsWidgetEnabled);
    s->Bind(16, guild.WidgetChannelID);
    s->Bind(17, guild.SystemChannelFlags);
    s->Bind(18, guild.RulesChannelID);
    s->Bind(19, guild.JoinedAt);
    s->Bind(20, guild.IsLarge);
    s->Bind(21, guild.IsUnavailable);
    s->Bind(22, guild.MemberCount);
    s->Bind(23, guild.MaxPresences);
    s->Bind(24, guild.MaxMembers);
    s->Bind(25, guild.VanityURL);
    s->Bind(26, guild.Description);
    s->Bind(27, guild.BannerHash);
    s->Bind(28, guild.PremiumTier);
    s->Bind(29, guild.PremiumSubscriptionCount);
    s->Bind(30, guild.PreferredLocale);
    s->Bind(31, guild.PublicUpdatesChannelID);
    s->Bind(32, guild.MaxVideoChannelUsers);
    s->Bind(33, guild.ApproximateMemberCount);
    s->Bind(34, guild.ApproximatePresenceCount);
    s->Bind(35, guild.IsLazy);

    if (!s->Insert())
        fprintf(stderr, "guild insert failed for %" PRIu64 ": %s\n", static_cast<uint64_t>(guild.ID), m_db.ErrStr());

    s->Reset();

    if (guild.Emojis.has_value()) {
        auto &s = m_stmt_set_guild_emoji;
        for (const auto &emoji : *guild.Emojis) {
            s->Bind(1, guild.ID);
            s->Bind(2, emoji.ID);
            if (!s->Insert())
                fprintf(stderr, "guild emoji insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(guild.ID), static_cast<uint64_t>(emoji.ID), m_db.ErrStr());
            s->Reset();
        }
    }

    if (guild.Features.has_value()) {
        auto &s = m_stmt_set_guild_feature;

        for (const auto &feature : *guild.Features) {
            s->Bind(1, guild.ID);
            s->Bind(2, feature);
            if (!s->Insert())
                fprintf(stderr, "guild feature insert failed for %" PRIu64 "/%s: %s\n", static_cast<uint64_t>(guild.ID), feature.c_str(), m_db.ErrStr());
            s->Reset();
        }
    }

    if (guild.Threads.has_value()) {
        auto &s = m_stmt_set_thread;

        for (const auto &thread : *guild.Threads) {
            s->Bind(1, guild.ID);
            s->Bind(2, thread.ID);
            if (!s->Insert())
                fprintf(stderr, "guild thread insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(guild.ID), static_cast<uint64_t>(thread.ID), m_db.ErrStr());
            s->Reset();
        }
    }

    EndTransaction();
}

void Store::SetGuildMember(Snowflake guild_id, Snowflake user_id, const GuildMember &data) {
    auto &s = m_stmt_set_member;

    s->Bind(1, user_id);
    s->Bind(2, guild_id);
    s->Bind(3, data.Nickname);
    s->Bind(4, data.JoinedAt);
    s->Bind(5, data.PremiumSince);
    s->Bind(6, data.IsDeafened);
    s->Bind(7, data.IsMuted);
    s->Bind(8, data.Avatar);
    s->Bind(9, data.IsPending);

    if (!s->Insert())
        fprintf(stderr, "member insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(user_id), static_cast<uint64_t>(guild_id), m_db.ErrStr());

    s->Reset();

    {
        auto &s = m_stmt_clr_member_roles;
        s->Bind(1, user_id);
        s->Bind(2, guild_id);
        s->Step();
        s->Reset();
    }

    {
        auto &s = m_stmt_set_member_roles;

        BeginTransaction();
        for (const auto &role : data.Roles) {
            s->Bind(1, user_id);
            s->Bind(2, role);
            if (!s->Insert())
                fprintf(stderr, "member role insert failed for %" PRIu64 "/%" PRIu64 "/%" PRIu64 ": %s\n",
                        static_cast<uint64_t>(user_id), static_cast<uint64_t>(guild_id), static_cast<uint64_t>(role), m_db.ErrStr());
            s->Reset();
        }
        EndTransaction();
    }
}

void Store::SetMessageInteractionPair(Snowflake message_id, const MessageInteractionData &interaction) {
    auto &s = m_stmt_set_interaction;

    s->Bind(1, message_id);
    s->Bind(2, interaction.ID);
    s->Bind(3, interaction.Type);
    s->Bind(4, interaction.Name);
    s->Bind(5, interaction.User.ID);

    if (!s->Insert())
        fprintf(stderr, "message interaction failed for %" PRIu64 ": %s\n", static_cast<uint64_t>(message_id), m_db.ErrStr());

    s->Reset();
}

void Store::SetMessage(Snowflake id, const Message &message) {
    auto &s = m_stmt_set_msg;

    BeginTransaction();

    s->Bind(1, id);
    s->Bind(2, message.ChannelID);
    s->Bind(3, message.GuildID);
    s->Bind(4, message.Author.ID);
    s->Bind(5, message.Content);
    s->Bind(6, message.Timestamp);
    s->Bind(7, message.EditedTimestamp);
    s->Bind(8, message.IsTTS);
    s->Bind(9, message.DoesMentionEveryone);
    s->BindAsJSON(10, message.Embeds);
    s->Bind(11, message.IsPinned);
    s->Bind(12, message.WebhookID);
    s->Bind(13, message.Type);
    s->BindAsJSON(14, message.Application);
    s->Bind(15, message.Flags);
    s->BindAsJSON(16, message.Stickers);
    s->Bind(17, message.IsDeleted());
    s->Bind(18, message.IsEdited());
    s->Bind(19, message.IsPending);
    s->Bind(20, message.Nonce);
    s->BindAsJSON(21, message.StickerItems);

    if (!s->Insert())
        fprintf(stderr, "message insert failed for %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());

    s->Reset();

    if (message.MessageReference.has_value()) {
        auto &s = m_stmt_set_msg_ref;
        s->Bind(1, message.ID);
        s->Bind(2, message.MessageReference->MessageID);
        s->Bind(3, message.MessageReference->ChannelID);
        s->Bind(4, message.MessageReference->GuildID);

        if (!s->Insert())
            fprintf(stderr, "message ref insert failed for %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());

        s->Reset();
    }

    for (const auto &u : message.Mentions) {
        auto &s = m_stmt_set_mention;
        s->Bind(1, id);
        s->Bind(2, u.ID);
        if (!s->Insert())
            fprintf(stderr, "message mention insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(id), static_cast<uint64_t>(u.ID), m_db.ErrStr());
        s->Reset();
    }

    for (const auto &r : message.MentionRoles) {
        auto &s = m_stmt_set_role_mention;
        s->Bind(1, id);
        s->Bind(2, r);
        if (!s->Insert())
            fprintf(stderr, "message role mention insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(id), static_cast<uint64_t>(r), m_db.ErrStr());
        s->Reset();
    }

    for (const auto &a : message.Attachments) {
        auto &s = m_stmt_set_attachment;
        s->Bind(1, id);
        s->Bind(2, a.ID);
        s->Bind(3, a.Filename);
        s->Bind(4, a.Bytes);
        s->Bind(5, a.URL);
        s->Bind(6, a.ProxyURL);
        s->Bind(7, a.Height);
        s->Bind(8, a.Width);
        s->Bind(9, a.Description);
        if (!s->Insert())
            fprintf(stderr, "message attachment insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(id), static_cast<uint64_t>(a.ID), m_db.ErrStr());
        s->Reset();
    }

    if (message.Reactions.has_value()) {
        auto &s = m_stmt_add_reaction;
        for (size_t i = 0; i < message.Reactions->size(); i++) {
            const auto &reaction = (*message.Reactions)[i];
            s->Bind(1, id);
            s->Bind(2, reaction.Emoji.ID);
            s->Bind(3, reaction.Emoji.Name);
            s->Bind(4, reaction.Count);
            s->Bind(5, reaction.HasReactedWith);
            s->Bind(6, i);
            if (!s->Insert())
                fprintf(stderr, "message reaction insert failed for %" PRIu64 "/%" PRIu64 "/%s: %s\n", static_cast<uint64_t>(id), static_cast<uint64_t>(reaction.Emoji.ID), reaction.Emoji.Name.c_str(), m_db.ErrStr());
            s->Reset();
        }
    }

    if (message.Interaction.has_value())
        SetMessageInteractionPair(id, *message.Interaction);

    EndTransaction();
}

void Store::SetPermissionOverwrite(Snowflake channel_id, Snowflake id, const PermissionOverwrite &perm) {
    auto &s = m_stmt_set_perm;

    s->Bind(1, perm.ID);
    s->Bind(2, channel_id);
    s->Bind(3, perm.Type);
    s->Bind(4, perm.Allow);
    s->Bind(5, perm.Deny);

    if (!s->Insert())
        fprintf(stderr, "permission insert failed for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(channel_id), static_cast<uint64_t>(id), m_db.ErrStr());

    s->Reset();
}

void Store::SetRole(Snowflake guild_id, const RoleData &role) {
    auto &s = m_stmt_set_role;

    s->Bind(1, role.ID);
    s->Bind(2, guild_id);
    s->Bind(3, role.Name);
    s->Bind(4, role.Color);
    s->Bind(5, role.IsHoisted);
    s->Bind(6, role.Position);
    s->Bind(7, role.Permissions);
    s->Bind(8, role.IsManaged);
    s->Bind(9, role.IsMentionable);

    if (!s->Insert())
        fprintf(stderr, "role insert failed for %" PRIu64 ": %s\n", static_cast<uint64_t>(role.ID), m_db.ErrStr());

    s->Reset();
}

void Store::SetUser(Snowflake id, const UserData &user) {
    auto &s = m_stmt_set_user;

    s->Bind(1, id);
    s->Bind(2, user.Username);
    s->Bind(3, user.Discriminator);
    s->Bind(4, user.Avatar);
    s->Bind(5, user.IsBot);
    s->Bind(6, user.IsSystem);
    s->Bind(7, user.IsMFAEnabled);
    s->Bind(8, user.PremiumType);
    s->Bind(9, user.PublicFlags);
    s->Bind(10, user.GlobalName);

    if (!s->Insert())
        fprintf(stderr, "user insert failed for %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());

    s->Reset();
}

std::optional<BanData> Store::GetBan(Snowflake guild_id, Snowflake user_id) const {
    auto &s = m_stmt_get_ban;

    s->Bind(1, guild_id);
    s->Bind(2, user_id);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "error while fetching ban for %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(guild_id), static_cast<uint64_t>(user_id), m_db.ErrStr());
        s->Reset();
        return {};
    }

    BanData r;
    r.User.ID = user_id;
    s->Get(2, r.Reason);

    s->Reset();

    return r;
}

std::vector<BanData> Store::GetBans(Snowflake guild_id) const {
    auto &s = m_stmt_get_bans;

    std::vector<BanData> ret;
    s->Bind(1, guild_id);
    while (s->FetchOne()) {
        auto &ban = ret.emplace_back();
        s->Get(1, ban.User.ID);
        s->Get(2, ban.Reason);
    }

    s->Reset();

    return ret;
}

std::optional<WebhookMessageData> Store::GetWebhookMessage(Snowflake message_id) const {
    auto &s = m_stmt_get_webhook_msg;

    s->Bind(1, message_id);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "error while fetching webhook message %" PRIu64 ": %s\n", static_cast<uint64_t>(message_id), m_db.ErrStr());
        s->Reset();
        return {};
    }

    WebhookMessageData data;
    s->Get(0, data.MessageID);
    s->Get(1, data.WebhookID);
    s->Get(2, data.Username);
    s->Get(3, data.Avatar);

    s->Reset();

    return data;
}

Snowflake Store::GetGuildOwner(Snowflake guild_id) const {
    auto &s = m_stmt_get_guild_owner;

    s->Bind(1, guild_id);
    if (s->FetchOne()) {
        Snowflake ret;
        s->Get(0, ret);
        s->Reset();
        return ret;
    }

    s->Reset();

    return Snowflake::Invalid;
}

std::vector<Snowflake> Store::GetMemberRoles(Snowflake guild_id, Snowflake user_id) const {
    std::vector<Snowflake> ret;

    auto &s = m_stmt_get_member_roles;

    s->Bind(1, user_id);
    s->Bind(2, guild_id);

    while (s->FetchOne()) {
        auto &f = ret.emplace_back();
        s->Get(0, f);
    }

    s->Reset();

    return ret;
}

std::vector<Message> Store::GetLastMessages(Snowflake id, size_t num) const {
    auto &s = m_stmt_get_last_msgs;
    std::vector<Message> msgs;
    s->Bind(1, id);
    s->Bind(2, num);
    while (s->FetchOne()) {
        auto msg = GetMessageBound(s);
        msgs.push_back(std::move(msg));
    }

    s->Reset();

    for (auto &msg : msgs) {
        if (msg.MessageReference.has_value() && msg.MessageReference->MessageID.has_value()) {
            auto ref = GetMessage(*msg.MessageReference->MessageID);
            if (ref.has_value())
                msg.ReferencedMessage = std::make_shared<Message>(std::move(*ref));
        }
    }

    return msgs;
}

std::vector<Message> Store::GetMessagesBefore(Snowflake channel_id, Snowflake message_id, size_t limit) const {
    std::vector<Message> msgs;

    auto &s = m_stmt_get_messages_before;

    s->Bind(1, channel_id);
    s->Bind(2, message_id);
    s->Bind(3, limit);

    while (s->FetchOne()) {
        auto msg = GetMessageBound(s);
        msgs.push_back(std::move(msg));
    }

    s->Reset();

    for (auto &msg : msgs) {
        if (msg.MessageReference.has_value() && msg.MessageReference->MessageID.has_value()) {
            auto ref = GetMessage(*msg.MessageReference->MessageID);
            if (ref.has_value()) {
                msg.ReferencedMessage = std::make_shared<Message>(std::move(*ref));
            }
        }
    }

    return msgs;
}

std::vector<Message> Store::GetPinnedMessages(Snowflake channel_id) const {
    std::vector<Message> msgs;

    auto &s = m_stmt_get_pins;

    s->Bind(1, channel_id);

    while (s->FetchOne()) {
        auto msg = GetMessageBound(s);
        msgs.push_back(std::move(msg));
    }

    s->Reset();

    for (auto &msg : msgs) {
        if (msg.MessageReference.has_value() && msg.MessageReference->MessageID.has_value()) {
            auto ref = GetMessage(*msg.MessageReference->MessageID);
            if (ref.has_value())
                msg.ReferencedMessage = std::make_shared<Message>(std::move(*ref));
        }
    }

    return msgs;
}

std::vector<ChannelData> Store::GetActiveThreads(Snowflake channel_id) const {
    std::vector<ChannelData> ret;

    auto &s = m_stmt_get_active_threads;

    s->Bind(1, channel_id);
    while (s->FetchOne()) {
        Snowflake x;
        s->Get(0, x);
        auto chan = GetChannel(x);
        if (chan.has_value())
            ret.push_back(*chan);
    }

    s->Reset();

    return ret;
}

std::vector<Snowflake> Store::GetChannelIDsWithParentID(Snowflake channel_id) const {
    auto &s = m_stmt_get_chan_ids_parent;

    s->Bind(1, channel_id);

    std::vector<Snowflake> ret;
    while (s->FetchOne()) {
        Snowflake x;
        s->Get(0, x);
        ret.push_back(x);
    }

    s->Reset();

    return ret;
}

std::unordered_set<Snowflake> Store::GetMembersInGuild(Snowflake guild_id) const {
    auto &s = m_stmt_get_guild_member_ids;

    s->Bind(1, guild_id);

    std::unordered_set<Snowflake> ret;
    while (s->FetchOne()) {
        Snowflake x;
        s->Get(0, x);
        ret.insert(x);
    }

    s->Reset();

    return ret;
}

void Store::AddReaction(const MessageReactionAddObject &data, bool byself) {
    auto &s = m_stmt_add_reaction;

    s->Bind(1, data.MessageID);
    s->Bind(2, data.Emoji.ID);
    s->Bind(3, data.Emoji.Name);
    s->Bind(4, 1);
    if (byself)
        s->Bind(5, true);
    else
        s->Bind(5);
    s->Bind(6);

    if (!s->Insert())
        fprintf(stderr, "failed to add reaction for %" PRIu64 ": %s\n", static_cast<uint64_t>(data.MessageID), m_db.ErrStr());

    s->Reset();
}

void Store::RemoveReaction(const MessageReactionRemoveObject &data, bool byself) {
    auto &s = m_stmt_sub_reaction;

    s->Bind(1, data.MessageID);
    s->Bind(2, data.Emoji.ID);
    s->Bind(3, data.Emoji.Name);
    if (byself)
        s->Bind(4, false);
    else
        s->Bind(4);

    if (!s->Insert())
        fprintf(stderr, "failed to remove reaction for %" PRIu64 ": %s\n", static_cast<uint64_t>(data.MessageID), m_db.ErrStr());

    s->Reset();
}

std::optional<ChannelData> Store::GetChannel(Snowflake id) const {
    auto &s = m_stmt_get_chan;
    s->Bind(1, id);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "error while fetching channel %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());
        s->Reset();
        return {};
    }

    ChannelData r;

    // uncomment as necessary
    r.ID = id;
    s->Get(1, r.Type);
    s->Get(2, r.GuildID);
    s->Get(3, r.Position);
    s->Get(4, r.Name);
    s->Get(5, r.Topic);
    s->Get(6, r.IsNSFW);
    s->Get(7, r.LastMessageID);
    s->Get(10, r.RateLimitPerUser);
    s->Get(11, r.Icon);
    s->Get(12, r.OwnerID);
    s->Get(14, r.ParentID);
    if (!s->IsNull(16)) {
        r.ThreadMetadata.emplace();
        s->Get(16, r.ThreadMetadata->IsArchived);
        s->Get(17, r.ThreadMetadata->AutoArchiveDuration);
        s->Get(18, r.ThreadMetadata->ArchiveTimestamp);
    }

    s->Reset();

    {
        auto &s = m_stmt_get_recipients;
        s->Bind(1, id);
        std::vector<Snowflake> recipients;
        while (s->FetchOne()) {
            auto &r = recipients.emplace_back();
            s->Get(0, r);
        }
        s->Reset();
        if (!recipients.empty())
            r.RecipientIDs = std::move(recipients);
    }

    return r;
}

std::optional<EmojiData> Store::GetEmoji(Snowflake id) const {
    auto &s = m_stmt_get_emoji;

    s->Bind(1, id);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "error while fetching emoji %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());
        s->Reset();
        return {};
    }

    EmojiData r;

    r.ID = id;
    s->Get(1, r.Name);
    if (!s->IsNull(2)) {
        r.Creator.emplace();
        s->Get(2, r.Creator->ID);
    }
    s->Get(3, r.NeedsColons);
    s->Get(4, r.IsManaged);
    s->Get(5, r.IsAnimated);
    s->Get(6, r.IsAvailable);

    {
        auto &s = m_stmt_get_emoji_roles;

        s->Bind(1, id);
        r.Roles.emplace();
        while (s->FetchOne()) {
            Snowflake id;
            s->Get(0, id);
            r.Roles->push_back(id);
        }
        s->Reset();
    }

    s->Reset();

    return r;
}

std::optional<GuildData> Store::GetGuild(Snowflake id) const {
    auto &s = m_stmt_get_guild;
    s->Bind(1, id);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "error while fetching guild %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());
        s->Reset();
        return {};
    }

    // unfetched fields arent used anywhere
    GuildData r;
    r.ID = id;
    s->Get(1, r.Name);
    s->Get(2, r.Icon);
    s->Get(5, r.OwnerID);
    s->Get(11, r.DefaultMessageNotifications);
    s->Get(20, r.IsUnavailable);
    s->Get(27, r.PremiumTier);

    s->Reset();

    {
        auto &s = m_stmt_get_guild_emojis;

        s->Bind(1, id);
        r.Emojis.emplace();
        while (s->FetchOne()) {
            auto &q = r.Emojis->emplace_back();
            s->Get(0, q.ID);
        }
        s->Reset();
    }

    {
        auto &s = m_stmt_get_guild_features;

        s->Bind(1, id);
        r.Features.emplace();
        while (s->FetchOne()) {
            std::string feature;
            s->Get(0, feature);
            r.Features->insert(feature);
        }
        s->Reset();
    }

    {
        auto &s = m_stmt_get_guild_chans;
        s->Bind(1, id);
        r.Channels.emplace();
        while (s->FetchOne()) {
            auto &q = r.Channels->emplace_back();
            s->Get(0, q.ID);
        }
        s->Reset();
    }

    {
        auto &s = m_stmt_get_threads;
        s->Bind(1, id);
        r.Threads.emplace();
        while (s->FetchOne()) {
            auto &q = r.Threads->emplace_back();
            s->Get(0, q.ID);
        }
        s->Reset();
    }

    {
        auto &s = m_stmt_get_guild_roles;
        s->Bind(1, id);
        r.Roles.emplace();
        while (s->FetchOne()) {
            r.Roles->push_back(GetRoleBound(s));
        }
        s->Reset();
    }

    return r;
}

std::optional<GuildMember> Store::GetGuildMember(Snowflake guild_id, Snowflake user_id) const {
    auto &s = m_stmt_get_member;

    s->Bind(1, user_id);
    s->Bind(2, guild_id);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "error while fetching member %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(user_id), static_cast<uint64_t>(guild_id), m_db.ErrStr());
        s->Reset();
        return {};
    }

    GuildMember r;
    r.User.emplace().ID = user_id;
    s->Get(2, r.Nickname);
    s->Get(3, r.JoinedAt);
    s->Get(4, r.PremiumSince);
    // s->Get(5, r.IsDeafened);
    // s->Get(6, r.IsMuted);
    s->Get(7, r.Avatar);
    s->Get(8, r.IsPending);

    s->Reset();

    {
        auto &s = m_stmt_get_member_roles;

        s->Bind(1, user_id);
        s->Bind(2, guild_id);

        while (s->FetchOne()) {
            auto &f = r.Roles.emplace_back();
            s->Get(0, f);
        }

        s->Reset();
    }

    return r;
}

std::optional<Message> Store::GetMessage(Snowflake id) const {
    auto &s = m_stmt_get_msg;

    s->Bind(1, id);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "error while fetching message %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());
        s->Reset();
        return {};
    }

    auto top = GetMessageBound(s);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "error while fetching message %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());
        s->Reset();
        return top;
    }

    auto ref = GetMessageBound(s);
    top.ReferencedMessage = std::make_shared<Message>(std::move(ref));

    s->Reset();

    return top;
}

UserData Store::GetUserBound(Statement *stmt) const {
    UserData u;
    stmt->Get(0, u.ID);
    stmt->Get(1, u.Username);
    stmt->Get(2, u.Discriminator);
    stmt->Get(3, u.Avatar);
    stmt->Get(4, u.IsBot);
    stmt->Get(5, u.IsSystem);
    stmt->Get(6, u.IsMFAEnabled);
    stmt->Get(7, u.PremiumType);
    stmt->Get(8, u.PublicFlags);
    stmt->Get(9, u.GlobalName);
    return u;
}

Message Store::GetMessageBound(std::unique_ptr<Statement> &s) const {
    Message r;

    s->Get(0, r.ID);
    s->Get(1, r.ChannelID);
    s->Get(2, r.GuildID);
    s->Get(3, r.Author.ID);
    s->Get(4, r.Content);
    s->Get(5, r.Timestamp);
    s->Get(6, r.EditedTimestamp);
    // s->Get(7, r.IsTTS);
    s->Get(8, r.DoesMentionEveryone);
    s->GetJSON(9, r.Embeds);
    s->Get(10, r.IsPinned);
    s->Get(11, r.WebhookID);
    s->Get(12, r.Type);
    s->GetJSON(13, r.Application);
    s->Get(14, r.Flags);
    s->GetJSON(15, r.Stickers);
    bool tmpb;
    s->Get(16, tmpb);
    if (tmpb) r.SetDeleted();
    s->Get(17, tmpb);
    if (tmpb) r.SetEdited();
    s->Get(18, r.IsPending);
    s->Get(19, r.Nonce);
    s->GetJSON(20, r.StickerItems);

    if (!s->IsNull(21)) {
        auto &i = r.Interaction.emplace();
        s->Get(21, i.ID);
        s->Get(22, i.Name);
        s->Get(23, i.Type);
        s->Get(24, i.User.ID);
    }

    if (!s->IsNull(25)) {
        auto &q = r.MessageReference.emplace();
        s->Get(25, q.MessageID);
        s->Get(26, q.ChannelID);
        s->Get(27, q.GuildID);
    }

    int num_attachments;
    s->Get(28, num_attachments);
    if (num_attachments > 0) {
        auto &s = m_stmt_get_attachments;
        s->Bind(1, r.ID);
        while (s->FetchOne()) {
            auto &q = r.Attachments.emplace_back();
            s->Get(1, q.ID);
            s->Get(2, q.Filename);
            s->Get(3, q.Bytes);
            s->Get(4, q.URL);
            s->Get(5, q.ProxyURL);
            s->Get(6, q.Height);
            s->Get(7, q.Width);
            s->Get(8, q.Description);
        }
        s->Reset();
    }

    {
        auto &s = m_stmt_get_mentions;
        s->Bind(1, r.ID);
        while (s->FetchOne()) {
            Snowflake id;
            s->Get(0, id);
            auto user = GetUser(id);
            if (user.has_value())
                r.Mentions.push_back(std::move(*user));
        }
        s->Reset();
    }

    {
        auto &s = m_stmt_get_role_mentions;
        s->Bind(1, r.ID);
        while (s->FetchOne()) {
            Snowflake id;
            s->Get(0, id);
            r.MentionRoles.push_back(id);
        }
        s->Reset();
    }

    {
        auto &s = m_stmt_get_reactions;
        s->Bind(1, r.ID);
        std::map<size_t, ReactionData> tmp;
        while (s->FetchOne()) {
            size_t idx;
            ReactionData q;
            s->Get(0, q.Count);
            s->Get(1, q.HasReactedWith);
            s->Get(2, idx);
            s->Get(3, q.Emoji.ID);
            s->Get(4, q.Emoji.Name);
            s->Get(5, q.Emoji.IsAnimated);
            tmp[idx] = q;
        }
        s->Reset();

        r.Reactions.emplace();
        for (const auto &[idx, reaction] : tmp)
            r.Reactions->push_back(reaction);
    }

    return r;
}

std::optional<PermissionOverwrite> Store::GetPermissionOverwrite(Snowflake channel_id, Snowflake id) const {
    auto &s = m_stmt_get_perm;

    s->Bind(1, id);
    s->Bind(2, channel_id);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "failed while fetching permission %" PRIu64 "/%" PRIu64 ": %s\n", static_cast<uint64_t>(channel_id), static_cast<uint64_t>(id), m_db.ErrStr());
        s->Reset();
        return {};
    }

    PermissionOverwrite r;
    r.ID = id;
    s->Get(2, r.Type);
    s->Get(3, r.Allow);
    s->Get(4, r.Deny);

    s->Reset();

    return r;
}

std::optional<RoleData> Store::GetRole(Snowflake id) const {
    auto &s = m_stmt_get_role;

    s->Bind(1, id);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "error while fetching role %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());
        s->Reset();
        return {};
    }

    auto role = GetRoleBound(s);

    s->Reset();

    return role;
}

RoleData Store::GetRoleBound(std::unique_ptr<Statement> &s) {
    RoleData r;

    s->Get(0, r.ID);
    // s->Get(1, guild id);
    s->Get(2, r.Name);
    s->Get(3, r.Color);
    s->Get(4, r.IsHoisted);
    s->Get(5, r.Position);
    s->Get(6, r.Permissions);
    s->Get(7, r.IsManaged);
    s->Get(8, r.IsMentionable);

    return r;
}

std::optional<UserData> Store::GetUser(Snowflake id) const {
    auto &s = m_stmt_get_user;
    s->Bind(1, id);
    if (!s->FetchOne()) {
        if (m_db.Error() != SQLITE_DONE)
            fprintf(stderr, "error while fetching user %" PRIu64 ": %s\n", static_cast<uint64_t>(id), m_db.ErrStr());
        s->Reset();
        return {};
    }

    auto r = GetUserBound(s.get());

    s->Reset();

    return r;
}

void Store::ClearGuild(Snowflake id) {
    auto &s = m_stmt_clr_guild;

    s->Bind(1, id);
    s->Step();
    s->Reset();
}

void Store::ClearChannel(Snowflake id) {
    auto &s = m_stmt_clr_chan;

    s->Bind(1, id);
    s->Step();
    s->Reset();
}

void Store::ClearBan(Snowflake guild_id, Snowflake user_id) {
    auto &s = m_stmt_clr_ban;

    s->Bind(1, guild_id);
    s->Bind(2, user_id);
    s->Step();
    s->Reset();
}

void Store::ClearRecipient(Snowflake channel_id, Snowflake user_id) {
    auto &s = m_stmt_clr_recipient;

    s->Bind(1, channel_id);
    s->Bind(2, user_id);
    s->Step();
    s->Reset();
}

void Store::ClearRole(Snowflake id) {
    auto &s = m_stmt_clr_role;

    s->Bind(1, id);
    s->Step();
    s->Reset();
}

std::unordered_set<Snowflake> Store::GetChannels() const {
    auto &s = m_stmt_get_chan_ids;
    std::unordered_set<Snowflake> r;

    while (s->FetchOne()) {
        Snowflake id;
        s->Get(0, id);
        r.insert(id);
    }

    s->Reset();

    return r;
}

std::unordered_set<Snowflake> Store::GetGuilds() const {
    auto &s = m_stmt_get_guild_ids;
    std::unordered_set<Snowflake> r;

    while (s->FetchOne()) {
        Snowflake id;
        s->Get(0, id);
        r.insert(id);
    }

    s->Reset();

    return r;
}

void Store::ClearAll() {
    if (m_db.Execute(R"(
        DELETE FROM attachments;
        DELETE FROM bans;
        DELETE FROM channels;
        DELETE FROM emojis;
        DELETE FROM emoji_roles;
        DELETE FROM guild_emojis;
        DELETE FROM guild_features;
        DELETE FROM guilds;
        DELETE FROM members;
        DELETE FROM member_roles;
        DELETE FROM mentions;
        DELETE FROM message_interactions;
        DELETE FROM message_references;
        DELETE FROM messages;
        DELETE FROM permissions;
        DELETE FROM reactions;
        DELETE FROM recipients;
        DELETE FROM roles;
        DELETE FROM threads;
        DELETE FROM users;
    )") != SQLITE_OK) {
        fprintf(stderr, "failed to clear: %s\n", m_db.ErrStr());
    }
}

void Store::BeginTransaction() {
    m_db.StartTransaction();
}

void Store::EndTransaction() {
    m_db.EndTransaction();
}

bool Store::CreateTables() {
    const char *create_users = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY,
            username TEXT NOT NULL,
            discriminator TEXT NOT NULL,
            avatar TEXT,
            bot BOOL,
            system BOOL,
            mfa BOOL,
            premium INTEGER,
            pubflags INTEGER,
            global_name TEXT
        )
    )";

    const char *create_permissions = R"(
        CREATE TABLE IF NOT EXISTS permissions (
            id INTEGER NOT NULL,
            channel_id INTEGER NOT NULL,
            type INTEGER NOT NULL,
            allow INTEGER NOT NULL,
            deny INTEGER NOT NULL,
            PRIMARY KEY(id, channel_id)
        )
    )";

    const char *create_messages = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY,
            channel_id INTEGER NOT NULL,
            guild_id INTEGER,
            author_id INTEGER NOT NULL,
            content TEXT NOT NULL,
            timestamp TEXT NOT NULL,
            edited_timestamp TEXT,
            tts BOOL NOT NULL,
            everyone BOOL NOT NULL,
            embeds TEXT NOT NULL, /* json */
            pinned BOOL,
            webhook_id INTEGER,
            type INTEGER,
            application TEXT, /* json */
            flags INTEGER,
            stickers TEXT, /* json */
            deleted BOOL, /* extra */
            edited BOOL, /* extra */
            pending BOOL, /* extra */
            nonce TEXT,
            sticker_items TEXT /* json */
        )
    )";

    const char *create_roles = R"(
        CREATE TABLE IF NOT EXISTS roles (
            id INTEGER PRIMARY KEY,
            guild INTEGER NOT NULL,
            name TEXT NOT NULL,
            color INTEGER NOT NULL,
            hoisted BOOL NOT NULL,
            position INTEGER NOT NULL,
            permissions INTEGER NOT NULL,
            managed BOOL NOT NULL,
            mentionable BOOL NOT NULL
        )
    )";

    const char *create_emojis = R"(
        CREATE TABLE IF NOT EXISTS emojis (
            id INTEGER PRIMARY KEY, /*though nullable, only custom emojis (with non-null ids) are stored*/
            name TEXT NOT NULL, /*same as id*/
            creator_id INTEGER,
            colons BOOL,
            managed BOOL,
            animated BOOL,
            available BOOL
        )
    )";

    const char *create_members = R"(
        CREATE TABLE IF NOT EXISTS members (
            user_id INTEGER NOT NULL,
            guild_id INTEGER NOT NULL,
            nickname TEXT,
            joined_at TEXT NOT NULL,
            premium_since TEXT,
            deaf BOOL NOT NULL,
            mute BOOL NOT NULL,
            avatar TEXT,
            pending BOOL,
            PRIMARY KEY(user_id, guild_id)
        )
    )";

    const char *create_guilds = R"(
        CREATE TABLE IF NOT EXISTS guilds (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            icon TEXT NOT NULL,
            splash TEXT,
            owner BOOL,
            owner_id INTEGER NOT NULL,
            permissions INTEGER, /* new */
            voice_region TEXT,
            afk_id INTEGER,
            afk_timeout INTEGER NOT NULL,
            verification INTEGER NOT NULL,
            notifications INTEGER NOT NULL,
            mfa INTEGER NOT NULL,
            application INTEGER,
            widget BOOL,
            widget_channel INTEGER,
            system_flags INTEGER NOT NULL,
            rules_channel INTEGER,
            joined_at TEXT,
            large BOOL,
            unavailable BOOL,
            member_count INTEGER,
            max_presences INTEGER,
            max_members INTEGER,
            vanity TEXT,
            description TEXT,
            banner_hash TEXT,
            premium_tier INTEGER NOT NULL,
            premium_count INTEGER,
            locale TEXT NOT NULL,
            public_updates_id INTEGER,
            max_video_users INTEGER,
            approx_members INTEGER,
            approx_presences INTEGER,
            lazy BOOL
        )
    )";

    const char *create_channels = R"(
        CREATE TABLE IF NOT EXISTS channels (
            id INTEGER PRIMARY KEY,
            type INTEGER NOT NULL,
            guild_id INTEGER,
            position INTEGER,
            name TEXT,
            topic TEXT,
            is_nsfw BOOL,
            last_message_id INTEGER,
            bitrate INTEGER,
            user_limit INTEGER,
            rate_limit INTEGER,
            icon TEXT,
            owner_id INTEGER,
            application_id INTEGER,
            parent_id INTEGER,
            last_pin_timestamp TEXT,
            archived BOOL, /* threads */
            auto_archive INTEGER, /* threads */
            archived_ts TEXT /* threads */
        )
    )";

    const char *create_bans = R"(
        CREATE TABLE IF NOT EXISTS bans (
            guild_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            reason TEXT,
            PRIMARY KEY(user_id, guild_id)
        )
    )";

    const char *create_interactions = R"(
        CREATE TABLE IF NOT EXISTS message_interactions (
            message_id INTEGER NOT NULL,
            interaction_id INTEGER NOT NULL,
            type INTEGER NOT NULL,
            name STRING NOT NULL,
            user_id INTEGER NOT NULL,
            PRIMARY KEY(message_id)
        )
    )";

    const char *create_references = R"(
        CREATE TABLE IF NOT EXISTS message_references (
            id INTEGER NOT NULL,
            message INTEGER,
            channel INTEGER,
            guild INTEGER,
            PRIMARY KEY(id)
        )
    )";

    const char *create_member_roles = R"(
        CREATE TABLE IF NOT EXISTS member_roles (
            user INTEGER NOT NULL,
            role INTEGER NOT NULL,
            PRIMARY KEY(user, role)
        )
    )";

    const char *create_guild_emojis = R"(
        CREATE TABLE IF NOT EXISTS guild_emojis (
            guild INTEGER NOT NULL,
            emoji INTEGER NOT NULL,
            PRIMARY KEY(guild, emoji)
        )
    )";

    const char *create_guild_features = R"(
        CREATE TABLE IF NOT EXISTS guild_features (
            guild INTEGER NOT NULL,
            feature CHAR(63) NOT NULL,
            PRIMARY KEY(guild, feature)
        )
    )";

    const char *create_threads = R"(
        CREATE TABLE IF NOT EXISTS threads (
            guild INTEGER NOT NULL,
            id INTEGER NOT NULL,
            PRIMARY KEY(guild, id)
        )
    )";

    const char *create_emoji_roles = R"(
        CREATE TABLE IF NOT EXISTS emoji_roles (
            emoji INTEGER NOT NULL,
            role INTEGER NOT NULL,
            PRIMARY KEY(emoji, role)
        )
    )";

    const char *create_mentions = R"(
        CREATE TABLE IF NOT EXISTS mentions (
            message INTEGER NOT NULL,
            user INTEGER NOT NULL,
            PRIMARY KEY(message, user)
        )
    )";

    const char *create_mention_roles = R"(
        CREATE TABLE IF NOT EXISTS mention_roles (
            message INTEGER NOT NULL,
            role INTEGER NOT NULL,
            PRIMARY KEY(message, role)
        )
    )";

    const char *create_attachments = R"(
        CREATE TABLE IF NOT EXISTS attachments (
            message INTEGER NOT NULL,
            id INTEGER NOT NULL,
            filename TEXT NOT NULL,
            size INTEGER NOT NULL,
            url TEXT NOT NULL,
            proxy TEXT NOT NULL,
            height INTEGER,
            width INTEGER,
            description TEXT,
            PRIMARY KEY(message, id)
        )
    )";

    const char *create_recipients = R"(
        CREATE TABLE IF NOT EXISTS recipients (
            channel INTEGER NOT NULL,
            user INTEGER NOT NULL,
            PRIMARY KEY(channel, user)
        )
    )";

    const char *create_reactions = R"(
        CREATE TABLE IF NOT EXISTS reactions (
            message INTEGER NOT NULL,
            emoji_id INTEGER,
            name TEXT NOT NULL,
            count INTEGER NOT NULL,
            me BOOL NOT NULL,
            idx INTEGER NOT NULL,
            PRIMARY KEY(message, emoji_id, name)
        )
    )";

    const char *create_webhook_messages = R"(
        CREATE TABLE IF NOT EXISTS webhook_messages (
            message_id INTEGER NOT NULL,
            webhook_id INTEGER NOT NULL,
            username TEXT,
            avatar TEXT
        )
    )";

    if (m_db.Execute(create_users) != SQLITE_OK) {
        fprintf(stderr, "failed to create user table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_permissions) != SQLITE_OK) {
        fprintf(stderr, "failed to create permissions table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_messages) != SQLITE_OK) {
        fprintf(stderr, "failed to create messages table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_roles) != SQLITE_OK) {
        fprintf(stderr, "failed to create roles table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_emojis) != SQLITE_OK) {
        fprintf(stderr, "failed to create emojis table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_members) != SQLITE_OK) {
        fprintf(stderr, "failed to create members table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_guilds) != SQLITE_OK) {
        fprintf(stderr, "failed to create guilds table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_channels) != SQLITE_OK) {
        fprintf(stderr, "failed to create channels table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_bans) != SQLITE_OK) {
        fprintf(stderr, "failed to create bans table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_interactions) != SQLITE_OK) {
        fprintf(stderr, "failed to create interactions table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_references) != SQLITE_OK) {
        fprintf(stderr, "failed to create references table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_member_roles) != SQLITE_OK) {
        fprintf(stderr, "failed to create member roles table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_guild_emojis) != SQLITE_OK) {
        fprintf(stderr, "failed to create guild emojis table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_guild_features) != SQLITE_OK) {
        fprintf(stderr, "failed to create guild features table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_threads) != SQLITE_OK) {
        fprintf(stderr, "failed to create threads table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_emoji_roles) != SQLITE_OK) {
        fprintf(stderr, "failed to create emoji roles table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_mentions) != SQLITE_OK) {
        fprintf(stderr, "failed to create mentions table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_mention_roles) != SQLITE_OK) {
        fprintf(stderr, "failed to create role mentions table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_attachments) != SQLITE_OK) {
        fprintf(stderr, "failed to create attachments table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_recipients) != SQLITE_OK) {
        fprintf(stderr, "failed to create recipients table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_reactions) != SQLITE_OK) {
        fprintf(stderr, "failed to create reactions table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(create_webhook_messages) != SQLITE_OK) {
        fprintf(stderr, "failed to create webhook messages table: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(R"(
        CREATE TRIGGER remove_zero_reactions AFTER UPDATE ON reactions WHEN new.count = 0
        BEGIN
            DELETE FROM reactions WHERE message = new.message AND emoji_id = new.emoji_id AND name = new.name;
        END
    )") != SQLITE_OK) {
        fprintf(stderr, "failed to create reactions trigger: %s\n", m_db.ErrStr());
        return false;
    }

    if (m_db.Execute(R"(
        CREATE TRIGGER remove_deleted_roles AFTER DELETE ON roles
        BEGIN
            DELETE FROM member_roles WHERE role = old.id;
        END
    )") != SQLITE_OK) {
        fprintf(stderr, "failed to create roles trigger: %s\n", m_db.ErrStr());
        return false;
    }

    return true;
}

bool Store::CreateStatements() {
    m_stmt_set_guild = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO guilds VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_guild->OK()) {
        fprintf(stderr, "failed to prepare set guild statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_guild = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM guilds WHERE id = ?
    )");
    if (!m_stmt_get_guild->OK()) {
        fprintf(stderr, "failed to prepare get guild statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_guild_ids = std::make_unique<Statement>(m_db, R"(
        SELECT id FROM guilds
    )");
    if (!m_stmt_get_guild_ids->OK()) {
        fprintf(stderr, "failed to prepare get guild ids statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_clr_guild = std::make_unique<Statement>(m_db, R"(
        DELETE FROM guilds WHERE id = ?
    )");
    if (!m_stmt_clr_guild->OK()) {
        fprintf(stderr, "failed to prepare clear guild statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_chan = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO channels VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_chan->OK()) {
        fprintf(stderr, "failed to prepare set channel statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_chan = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM channels WHERE id = ?
    )");
    if (!m_stmt_get_chan->OK()) {
        fprintf(stderr, "failed to prepare get channel statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_chan_ids = std::make_unique<Statement>(m_db, R"(
        SELECT id FROM channels
    )");
    if (!m_stmt_get_chan_ids->OK()) {
        fprintf(stderr, "failed to prepare get channel ids statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_clr_chan = std::make_unique<Statement>(m_db, R"(
        DELETE FROM channels WHERE id = ?
    )");
    if (!m_stmt_clr_chan->OK()) {
        fprintf(stderr, "failed to prepare clear channel statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_msg = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO messages VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_msg->OK()) {
        fprintf(stderr, "failed to prepare set message statement: %s\n", m_db.ErrStr());
        return false;
    }

    // wew
    m_stmt_get_msg = std::make_unique<Statement>(m_db, R"(
        SELECT messages.*,
               message_interactions.interaction_id,
               message_interactions.name,
               message_interactions.type,
               message_interactions.user_id,
               message_references.message,
               message_references.channel,
               message_references.guild,
               COUNT(attachments.id)
        FROM messages
        LEFT OUTER JOIN
            message_interactions
                ON messages.id = message_interactions.message_id
        LEFT OUTER JOIN
            message_references
                ON messages.id = message_references.id
        LEFT JOIN
            attachments
                ON messages.id = attachments.message
        WHERE messages.id = ?1 GROUP BY messages.id
        UNION ALL
        SELECT messages.*,
               message_interactions.interaction_id,
               message_interactions.name,
               message_interactions.type,
               message_interactions.user_id,
               message_references.message,
               message_references.channel,
               message_references.guild,
               COUNT(attachments.id)
        FROM messages
        LEFT OUTER JOIN
            message_interactions
                ON messages.id = message_interactions.message_id
        LEFT OUTER JOIN
            message_references
                ON messages.id = message_references.id
        LEFT JOIN
            attachments
                ON messages.id = attachments.message
        WHERE messages.id = (SELECT message FROM message_references WHERE id = ?1)
        GROUP BY messages.id ORDER BY messages.id DESC
    )");
    if (!m_stmt_get_msg->OK()) {
        fprintf(stderr, "failed to prepare get message statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_msg_ref = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO message_references VALUES (
            ?, ?, ?, ?
        );
    )");
    if (!m_stmt_set_msg_ref->OK()) {
        fprintf(stderr, "failed to prepare set message reference statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_last_msgs = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM (
            SELECT messages.*,
                   message_interactions.interaction_id,
                   message_interactions.name,
                   message_interactions.type,
                   message_interactions.user_id,
                   message_references.message,
                   message_references.channel,
                   message_references.guild,
                   COUNT(attachments.id)
            FROM messages
            LEFT OUTER JOIN
                message_interactions
                    ON messages.id = message_interactions.message_id
            LEFT OUTER JOIN
                message_references
                    ON messages.id = message_references.id
            LEFT JOIN
                attachments
                    ON messages.id = attachments.message
            WHERE channel_id = ? AND pending = 0 GROUP BY messages.id ORDER BY id DESC LIMIT ?
        ) ORDER BY id ASC
    )");
    if (!m_stmt_get_last_msgs->OK()) {
        fprintf(stderr, "failed to prepare get last messages statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_user = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO users VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_user->OK()) {
        fprintf(stderr, "failed to prepare set user statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_user = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM users WHERE id = ?
    )");
    if (!m_stmt_get_user->OK()) {
        fprintf(stderr, "failed to prepare get user statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_member = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO members VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_member->OK()) {
        fprintf(stderr, "failed to prepare set member statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_member = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM members WHERE user_id = ? AND guild_id = ?
    )");
    if (!m_stmt_get_member->OK()) {
        fprintf(stderr, "failed to prepare get member statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_role = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO roles VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_role->OK()) {
        fprintf(stderr, "failed to prepare set role statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_role = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM roles WHERE id = ?
    )");
    if (!m_stmt_get_role->OK()) {
        fprintf(stderr, "failed to prepare get role statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_guild_roles = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM roles WHERE guild = ?
    )");
    if (!m_stmt_get_guild_roles->OK()) {
        fprintf(stderr, "failed to prepare get guild roles statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_emoji = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO emojis VALUES (
            ?, ?, ?, ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_emoji->OK()) {
        fprintf(stderr, "failed to prepare set emoji statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_emoji = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM emojis WHERE id = ?
    )");
    if (!m_stmt_get_emoji->OK()) {
        fprintf(stderr, "failed to prepare get emoji statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_perm = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO permissions VALUES (
            ?, ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_perm->OK()) {
        fprintf(stderr, "failed to prepare set permission statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_perm = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM permissions WHERE id = ? AND channel_id = ?
    )");
    if (!m_stmt_get_perm->OK()) {
        fprintf(stderr, "failed to prepare get permission statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_ban = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO bans VALUES (
            ?, ?, ?
        )
    )");
    if (!m_stmt_set_ban->OK()) {
        fprintf(stderr, "failed to prepare set ban statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_ban = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM bans WHERE guild_id = ? AND user_id = ?
    )");
    if (!m_stmt_get_ban->OK()) {
        fprintf(stderr, "failed to prepare get ban statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_bans = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM bans WHERE guild_id = ?
    )");
    if (!m_stmt_get_bans->OK()) {
        fprintf(stderr, "failed to prepare get bans statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_clr_ban = std::make_unique<Statement>(m_db, R"(
        DELETE FROM bans WHERE guild_id = ? AND user_id = ?
    )");
    if (!m_stmt_clr_ban->OK()) {
        fprintf(stderr, "failed to prepare clear ban statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_interaction = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO message_interactions VALUES (
            ?, ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_interaction->OK()) {
        fprintf(stderr, "failed to prepare set interaction statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_member_roles = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO member_roles VALUES (
            ?, ?
        )
    )");
    if (!m_stmt_set_member_roles->OK()) {
        fprintf(stderr, "faile to prepare set member roles statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_member_roles = std::make_unique<Statement>(m_db, R"(
        SELECT id FROM roles, member_roles
        WHERE roles.id = member_roles.role
        AND member_roles.user = ?
        AND roles.guild = ?
    )");
    if (!m_stmt_get_member_roles->OK()) {
        fprintf(stderr, "failed to prepare get member role statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_clr_member_roles = std::make_unique<Statement>(m_db, R"(
        DELETE FROM member_roles
        WHERE user = ? AND
        EXISTS (
            SELECT 1 FROM roles
            WHERE member_roles.role = roles.id
            AND roles.guild = ?
        )
    )");
    if (!m_stmt_clr_member_roles->OK()) {
        fprintf(stderr, "failed to prepare clear member roles statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_guild_emoji = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO guild_emojis VALUES (
            ?, ?
        )
    )");
    if (!m_stmt_set_guild_emoji->OK()) {
        fprintf(stderr, "failed to prepare set guild emoji statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_guild_emojis = std::make_unique<Statement>(m_db, R"(
        SELECT emoji FROM guild_emojis WHERE guild = ?
    )");
    if (!m_stmt_get_guild_emojis->OK()) {
        fprintf(stderr, "failed to prepare get guild emojis statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_clr_guild_emoji = std::make_unique<Statement>(m_db, R"(
        DELETE FROM guild_emojis WHERE guild = ? AND emoji = ?
    )");
    if (!m_stmt_clr_guild_emoji->OK()) {
        fprintf(stderr, "failed to prepare clear guild emoji statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_guild_feature = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO guild_features VALUES (
            ?, ?
        )
    )");
    if (!m_stmt_set_guild_feature->OK()) {
        fprintf(stderr, "failed to prepare set guild feature statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_guild_features = std::make_unique<Statement>(m_db, R"(
        SELECT feature FROM guild_features WHERE guild = ?  
    )");
    if (!m_stmt_get_guild_features->OK()) {
        fprintf(stderr, "failed to prepare get guild features statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_guild_chans = std::make_unique<Statement>(m_db, R"(
        SELECT id FROM channels WHERE guild_id = ?
    )");
    if (!m_stmt_get_guild_chans->OK()) {
        fprintf(stderr, "failed to prepare get guild channels statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_thread = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO threads VALUES (
            ?, ?
        )
    )");
    if (!m_stmt_set_thread->OK()) {
        fprintf(stderr, "failed to prepare set thread statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_threads = std::make_unique<Statement>(m_db, R"(
        SELECT id FROM threads WHERE guild = ?
    )");
    if (!m_stmt_get_threads->OK()) {
        fprintf(stderr, "failed to prepare get threads statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_active_threads = std::make_unique<Statement>(m_db, R"(
        SELECT id FROM channels WHERE parent_id = ? AND (type = 10 OR type = 11 OR type = 12) AND archived = FALSE
    )");
    if (!m_stmt_get_active_threads->OK()) {
        fprintf(stderr, "faile to prepare get active threads statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_messages_before = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM (
            SELECT messages.*,
                   message_interactions.interaction_id,
                   message_interactions.name,
                   message_interactions.type,
                   message_interactions.user_id,
                   message_references.message,
                   message_references.channel,
                   message_references.guild,
                   COUNT(attachments.id)
            FROM messages
            LEFT OUTER JOIN
                message_interactions
                    ON messages.id = message_interactions.message_id
            LEFT OUTER JOIN
                message_references
                    ON messages.id = message_references.id
            LEFT OUTER JOIN
                attachments
                    ON messages.id = attachments.message
            WHERE channel_id = ? AND pending = 0 AND messages.id < ? ORDER BY id DESC LIMIT ?
        ) WHERE id IS NOT NULL ORDER BY id ASC
    )");
    if (!m_stmt_get_messages_before->OK()) {
        fprintf(stderr, "failed to prepare get messages before statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_pins = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM (
            SELECT messages.*,
                   message_interactions.interaction_id,
                   message_interactions.name,
                   message_interactions.type,
                   message_interactions.user_id,
                   message_references.message,
                   message_references.channel,
                   message_references.guild,
                   COUNT(attachments.id)
            FROM messages
            LEFT OUTER JOIN
                message_interactions
                    ON messages.id = message_interactions.message_id
            LEFT OUTER JOIN
                message_references
                    ON messages.id = message_references.id
            LEFT OUTER JOIN
                attachments
                    ON messages.id = attachments.message
            WHERE channel_id = ? AND pinned = 1 ORDER BY id ASC
        ) WHERE id IS NOT NULL
    )");
    if (!m_stmt_get_pins->OK()) {
        fprintf(stderr, "failed to prepare get pins statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_emoji_role = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO emoji_roles VALUES (
            ?, ?
        )
    )");
    if (!m_stmt_set_emoji_role->OK()) {
        fprintf(stderr, "failed to prepare set emoji role statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_emoji_roles = std::make_unique<Statement>(m_db, R"(
        SELECT role FROM emoji_roles WHERE emoji = ?
    )");
    if (!m_stmt_get_emoji_roles->OK()) {
        fprintf(stderr, "failed to prepare get emoji role statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_mention = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO mentions VALUES (
            ?, ?
        )
    )");
    if (!m_stmt_set_mention->OK()) {
        fprintf(stderr, "failed to prepare set mention statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_mentions = std::make_unique<Statement>(m_db, R"(
        SELECT user FROM mentions WHERE message = ?
    )");
    if (!m_stmt_get_mentions->OK()) {
        fprintf(stderr, "failed to prepare get mentions statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_role_mention = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO mention_roles VALUES (
            ?, ?
        )
    )");
    if (!m_stmt_set_role_mention->OK()) {
        fprintf(stderr, "failed to prepare set role mention statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_role_mentions = std::make_unique<Statement>(m_db, R"(
        SELECT role FROM mention_roles WHERE message = ?
    )");
    if (!m_stmt_get_role_mentions->OK()) {
        fprintf(stderr, "failed to prepare get role mentions statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_attachment = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO attachments VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_attachment->OK()) {
        fprintf(stderr, "failed to prepare set attachment statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_attachments = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM attachments WHERE message = ?
    )");
    if (!m_stmt_get_attachments->OK()) {
        fprintf(stderr, "failed to prepare get attachments statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_recipient = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO recipients VALUES (
            ?, ?
        )
    )");
    if (!m_stmt_set_recipient->OK()) {
        fprintf(stderr, "failed to prepare set recipient statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_recipients = std::make_unique<Statement>(m_db, R"(
        SELECT user FROM recipients WHERE channel = ?
    )");
    if (!m_stmt_get_recipients->OK()) {
        fprintf(stderr, "failed to prepare get recipients statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_clr_recipient = std::make_unique<Statement>(m_db, R"(
        DELETE FROM recipients WHERE channel = ? AND user = ?
    )");
    if (!m_stmt_clr_recipient->OK()) {
        fprintf(stderr, "failed to prepare clear recipient statement: %s\n", m_db.ErrStr());
        return false;
    }

    // probably not the best way to do this lol but i just want one statement i guess
    m_stmt_add_reaction = std::make_unique<Statement>(m_db, R"(
        INSERT OR REPLACE INTO reactions VALUES (
            ?1, ?2, ?3,
            COALESCE(
                (SELECT count FROM reactions WHERE message = ?1 AND emoji_id = ?2 AND name = ?3),
                0
            ) + ?4,
            COALESCE(
                ?5,
                (SELECT me FROM reactions WHERE message = ?1 AND emoji_id = ?2 AND name = ?3),
                false
            ),
            COALESCE(
                ?6,
                (SELECT idx FROM reactions WHERE message = ?1 AND emoji_id = ?2 AND name = ?3),
                (SELECT MAX(idx) + 1 FROM reactions WHERE message = ?1),
                0
            )
        )
    )");
    if (!m_stmt_add_reaction->OK()) {
        fprintf(stderr, "failed to prepare add reaction statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_sub_reaction = std::make_unique<Statement>(m_db, R"(
        UPDATE reactions
        SET count = count - 1,
            me = COALESCE(?4, me)
        WHERE message = ?1 AND emoji_id = ?2 AND name = ?3
    )");
    if (!m_stmt_sub_reaction->OK()) {
        fprintf(stderr, "failed to prepare sub reaction statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_reactions = std::make_unique<Statement>(m_db, R"(
        SELECT
            reactions.count,
            reactions.me,
            reactions.idx,
            emojis.id,
            emojis.name,
            emojis.animated
        FROM
            reactions
        INNER JOIN
            emojis ON reactions.emoji_id = emojis.id
        WHERE
            message = ?
    )");
    if (!m_stmt_get_reactions->OK()) {
        fprintf(stderr, "failed to prepare get reactions statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_chan_ids_parent = std::make_unique<Statement>(m_db, R"(
        SELECT id FROM channels WHERE parent_id = ?
    )");
    if (!m_stmt_get_chan_ids_parent->OK()) {
        fprintf(stderr, "failed to prepare get channel ids for parent statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_guild_member_ids = std::make_unique<Statement>(m_db, R"(
        SELECT user_id FROM members WHERE guild_id = ?
    )");
    if (!m_stmt_get_guild_member_ids->OK()) {
        fprintf(stderr, "failed to prepare get guild member ids statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_clr_role = std::make_unique<Statement>(m_db, R"(
        DELETE FROM roles
        WHERE id = ?1;
    )");
    if (!m_stmt_clr_role->OK()) {
        fprintf(stderr, "failed to prepare clear role statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_guild_owner = std::make_unique<Statement>(m_db, R"(
        SELECT owner_id FROM guilds WHERE id = ?
    )");
    if (!m_stmt_get_guild_owner->OK()) {
        fprintf(stderr, "failed to prepare get guild owner statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_set_webhook_msg = std::make_unique<Statement>(m_db, R"(
        REPLACE INTO webhook_messages VALUES (
            ?, ?, ?, ?
        )
    )");
    if (!m_stmt_set_webhook_msg->OK()) {
        fprintf(stderr, "failed to prepare set webhook message statement: %s\n", m_db.ErrStr());
        return false;
    }

    m_stmt_get_webhook_msg = std::make_unique<Statement>(m_db, R"(
        SELECT * FROM webhook_messages WHERE message_id = ?
    )");
    if (!m_stmt_get_webhook_msg->OK()) {
        fprintf(stderr, "failed to prepare get webhook message statement: %s\n", m_db.ErrStr());
        return false;
    }

    return true;
}

Store::Database::Database(const char *path)
    : m_db_path(path) {
    if (path != ":memory:"s) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec) && !std::filesystem::remove(path, ec)) {
            fprintf(stderr, "the database could not be removed. the database may be corrupted as a result\n");
        }
    }

    m_err = sqlite3_open(path, &m_db);
}

Store::Database::~Database() {
    Close();
}

int Store::Database::Close() {
    if (m_db == nullptr) return m_err;
    m_err = sqlite3_close(m_db);
    m_db = nullptr;

    if (!OK()) {
        fprintf(stderr, "error closing database: %s\n", ErrStr());
    } else {
        if (m_db_path != ":memory:") {
            std::error_code ec;
            std::filesystem::remove(m_db_path, ec);
        }
    }

    return m_err;
}

int Store::Database::StartTransaction() {
    return m_err = Execute("BEGIN TRANSACTION");
}

int Store::Database::EndTransaction() {
    return m_err = Execute("COMMIT");
}

int Store::Database::Execute(const char *command) {
    return m_err = sqlite3_exec(m_db, command, nullptr, nullptr, nullptr);
}

int Store::Database::Error() const {
    return m_err;
}

bool Store::Database::OK() const {
    return Error() == SQLITE_OK;
}

const char *Store::Database::ErrStr() const {
    const char *errstr = sqlite3_errstr(m_err);
    const char *errmsg = sqlite3_errmsg(m_db);
    std::string tmp = errstr + std::string("\n\t") + errmsg;
    tmp.copy(m_err_scratch, sizeof(m_err_scratch) - 1);
    m_err_scratch[std::min(tmp.size(), sizeof(m_err_scratch) - 1)] = '\0';
    return m_err_scratch;
}

int Store::Database::SetError(int err) {
    return m_err = err;
}

sqlite3 *Store::Database::obj() {
    return m_db;
}

Store::Statement::Statement(Database &db, const char *command)
    : m_db(&db) {
    if (m_db->SetError(sqlite3_prepare_v2(m_db->obj(), command, -1, &m_stmt, nullptr)) != SQLITE_OK) return;
}

Store::Statement::~Statement() {
    sqlite3_finalize(m_stmt);
}

bool Store::Statement::OK() const {
    return m_stmt != nullptr;
}

int Store::Statement::Bind(int index, Snowflake id) {
    return Bind(index, static_cast<uint64_t>(id));
}

int Store::Statement::Bind(int index, const char *str, size_t len) {
    if (len == -1) len = strlen(str);
    return m_db->SetError(sqlite3_bind_blob(m_stmt, index, str, static_cast<int>(len), SQLITE_TRANSIENT));
}

int Store::Statement::Bind(int index, const std::string &str) {
    return m_db->SetError(sqlite3_bind_blob(m_stmt, index, str.c_str(), static_cast<int>(str.size()), SQLITE_TRANSIENT));
}

int Store::Statement::Bind(int index) {
    return m_db->SetError(sqlite3_bind_null(m_stmt, index));
}

void Store::Statement::Get(int index, Snowflake &out) const {
    out = static_cast<uint64_t>(sqlite3_column_int64(m_stmt, index));
}

void Store::Statement::Get(int index, std::string &out) const {
    const unsigned char *ptr = sqlite3_column_text(m_stmt, index);
    if (ptr == nullptr)
        out = "";
    else
        out = reinterpret_cast<const char *>(ptr);
}

bool Store::Statement::IsNull(int index) const {
    return sqlite3_column_type(m_stmt, index) == SQLITE_NULL;
}

int Store::Statement::Step() {
    return m_db->SetError(sqlite3_step(m_stmt));
}

bool Store::Statement::Insert() {
    return m_db->SetError(sqlite3_step(m_stmt)) == SQLITE_DONE;
}

bool Store::Statement::FetchOne() {
    return m_db->SetError(sqlite3_step(m_stmt)) == SQLITE_ROW;
}

int Store::Statement::Reset() {
    if (m_db->SetError(sqlite3_reset(m_stmt)) != SQLITE_OK)
        return m_db->Error();
    if (m_db->SetError(sqlite3_clear_bindings(m_stmt)) != SQLITE_OK)
        return m_db->Error();
    return m_db->Error();
}

sqlite3_stmt *Store::Statement::obj() {
    return m_stmt;
}
