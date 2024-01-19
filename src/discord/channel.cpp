#include "channel.hpp"

#include "abaddon.hpp"

void from_json(const nlohmann::json &j, ThreadMetadataData &m) {
    JS_D("archived", m.IsArchived);
    JS_D("auto_archive_duration", m.AutoArchiveDuration);
    JS_D("archive_timestamp", m.ArchiveTimestamp);
    JS_O("locked", m.IsLocked);
}

void from_json(const nlohmann::json &j, ThreadMemberObject &m) {
    JS_O("id", m.ThreadID);
    JS_O("user_id", m.UserID);
    JS_D("join_timestamp", m.JoinTimestamp);
    JS_D("flags", m.Flags);
    JS_O("muted", m.IsMuted);
    JS_ON("mute_config", m.MuteConfig);
}

void from_json(const nlohmann::json &j, ChannelData &m) {
    JS_D("id", m.ID);
    JS_D("type", m.Type);
    JS_O("guild_id", m.GuildID);
    JS_O("position", m.Position);
    JS_O("permission_overwrites", m.PermissionOverwrites);
    JS_ON("name", m.Name);
    JS_ON("topic", m.Topic);
    JS_O("nsfw", m.IsNSFW);
    JS_ON("last_message_id", m.LastMessageID);
    JS_O("bitrate", m.Bitrate);
    JS_O("user_limit", m.UserLimit);
    JS_O("rate_limit_per_user", m.RateLimitPerUser);
    JS_O("recipients", m.Recipients);
    JS_O("recipient_ids", m.RecipientIDs);
    JS_ON("icon", m.Icon);
    JS_O("owner_id", m.OwnerID);
    JS_O("application_id", m.ApplicationID);
    JS_ON("parent_id", m.ParentID);
    JS_ON("last_pin_timestamp", m.LastPinTimestamp);
    JS_O("thread_metadata", m.ThreadMetadata);
    JS_O("member", m.ThreadMember);
}

void ChannelData::update_from_json(const nlohmann::json &j) {
    JS_RD("type", Type);
    JS_RD("guild_id", GuildID);
    JS_RV("position", Position, -1);
    JS_RD("permission_overwrites", PermissionOverwrites);
    JS_RD("name", Name);
    JS_RD("topic", Topic);
    JS_RD("nsfw", IsNSFW);
    JS_RD("last_message_id", LastMessageID);
    JS_RD("bitrate", Bitrate);
    JS_RD("user_limit", UserLimit);
    JS_RD("rate_limit_per_user", RateLimitPerUser);
    JS_RD("recipients", Recipients);
    JS_RD("icon", Icon);
    JS_RD("owner_id", OwnerID);
    JS_RD("application_id", ApplicationID);
    JS_RD("parent_id", ParentID);
    JS_RD("last_pin_timestamp", LastPinTimestamp);
}

bool ChannelData::NSFW() const {
    return IsNSFW.has_value() && *IsNSFW;
}

bool ChannelData::IsDM() const noexcept {
    return Type == ChannelType::DM ||
           Type == ChannelType::GROUP_DM;
}

bool ChannelData::IsThread() const noexcept {
    return Type == ChannelType::GUILD_PUBLIC_THREAD ||
           Type == ChannelType::GUILD_PRIVATE_THREAD ||
           Type == ChannelType::GUILD_NEWS_THREAD;
}

bool ChannelData::IsJoinedThread() const {
    return Abaddon::Get().GetDiscordClient().IsThreadJoined(ID);
}

bool ChannelData::IsCategory() const noexcept {
    return Type == ChannelType::GUILD_CATEGORY;
}

bool ChannelData::IsText() const noexcept {
    return Type == ChannelType::GUILD_TEXT || Type == ChannelType::GUILD_NEWS;
}

bool ChannelData::HasIcon() const noexcept {
    return Icon.has_value();
}

std::string ChannelData::GetIconURL() const {
    if (HasIcon()) {
        return "https://cdn.discordapp.com/channel-icons/" + std::to_string(ID) + "/" + *Icon + ".png";
    } else {
        const auto recipients = GetDMRecipients();
        if (!recipients.empty())
            return recipients[0].GetAvatarURL("png", "32");
        else
            return "https://cdn.discordapp.com/embed/avatars/0.png";
    }
}

std::string ChannelData::GetDisplayName() const {
    if (Name.has_value()) {
        if (IsDM()) {
            return *Name;
        } else {
            return "#" + *Name;
        }
    } else {
        return GetRecipientsDisplay();
    }
}

std::vector<Snowflake> ChannelData::GetChildIDs() const {
    return Abaddon::Get().GetDiscordClient().GetChildChannelIDs(ID);
}

std::optional<PermissionOverwrite> ChannelData::GetOverwrite(Snowflake id) const {
    return Abaddon::Get().GetDiscordClient().GetPermissionOverwrite(ID, id);
}

std::vector<UserData> ChannelData::GetDMRecipients() const {
    const auto &discord = Abaddon::Get().GetDiscordClient();
    if (Recipients.has_value())
        return *Recipients;

    if (RecipientIDs.has_value()) {
        std::vector<UserData> ret;
        for (const auto &id : *RecipientIDs) {
            auto user = discord.GetUser(id);
            if (user.has_value())
                ret.push_back(std::move(*user));
        }

        return ret;
    }

    return {};
}

std::string ChannelData::GetRecipientsDisplay() const {
    const auto self_id = Abaddon::Get().GetDiscordClient().GetUserData().ID;
    const auto recipients = GetDMRecipients();

    if (Type == ChannelType::DM && !recipients.empty()) {
        return recipients[0].GetDisplayName();
    }

    Glib::ustring r;
    for (size_t i = 0; i < recipients.size(); i++) {
        const auto &recipient = recipients[i];
        r += recipient.GetDisplayName();
        if (i < recipients.size() - 1) {
            r += ", ";
        }
    }

    if (r.empty()) r = "Unnamed";

    return r;
}
