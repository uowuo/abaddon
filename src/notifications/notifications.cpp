#include "notifications.hpp"
#include "discord/message.hpp"

Notifications::Notifications() {
}

bool CheckGuildMessage(const Message &message) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (!message.GuildID.has_value()) return false;

    const auto guild = discord.GetGuild(*message.GuildID);
    if (!guild.has_value()) return false;

    const auto guild_settings = discord.GetSettingsForGuild(*message.GuildID);

    // if theres no guild settings then just use default message notifications level
    // (there will be no channel settings)
    if (!guild_settings.has_value()) {
        if (guild->DefaultMessageNotifications.has_value()) {
            switch (*guild->DefaultMessageNotifications) {
                case DefaultNotificationLevel::ALL_MESSAGES:
                    return true;
                case DefaultNotificationLevel::ONLY_MENTIONS:
                    return message.DoesMentionEveryoneOrUser(discord.GetUserData().ID);
                default:
                    return false;
            }
        }
        return false;
    } else if (guild_settings->Muted) {
        // if there are guild settings and the guild is muted then dont notify
        return false;
    }

    // if the channel category is muted then dont notify
    const auto channel = discord.GetChannel(message.ChannelID);
    std::optional<UserGuildSettingsChannelOverride> category_settings;
    if (channel.has_value() && channel->ParentID.has_value()) {
        category_settings = guild_settings->GetOverride(*channel->ParentID);
        if (category_settings.has_value() && category_settings->Muted) {
            return false;
        }
    }

    const auto channel_settings = guild_settings->GetOverride(message.ChannelID);

    // 🥴
    NotificationLevel level;
    if (channel_settings.has_value()) {
        if (channel_settings->MessageNotifications == NotificationLevel::USE_UPPER) {
            if (category_settings.has_value()) {
                if (category_settings->MessageNotifications == NotificationLevel::USE_UPPER) {
                    level = guild_settings->MessageNotifications;
                } else {
                    level = category_settings->MessageNotifications;
                }
            } else {
                level = guild_settings->MessageNotifications;
            }
        } else {
            level = channel_settings->MessageNotifications;
        }
    } else {
        if (category_settings.has_value()) {
            if (category_settings->MessageNotifications == NotificationLevel::USE_UPPER) {
                level = guild_settings->MessageNotifications;
            } else {
                level = category_settings->MessageNotifications;
            }
        } else {
            level = guild_settings->MessageNotifications;
        }
    }

    // there are channel settings, so use them
    switch (level) {
        case NotificationLevel::ALL_MESSAGES:
            return true;
        case NotificationLevel::ONLY_MENTIONS:
            return message.DoesMentionEveryoneOrUser(discord.GetUserData().ID);
        case NotificationLevel::NO_MESSAGES:
            return false;
        default:
            return false;
    }
}

void Notifications::CheckMessage(const Message &message) {
    // ignore if our status is do not disturb
    if (IsDND()) return;
    auto &discord = Abaddon::Get().GetDiscordClient();
    // ignore if the channel is muted
    if (discord.IsChannelMuted(message.ChannelID)) return;
    // ignore if focused and message's channel is active
    if (Abaddon::Get().IsMainWindowActive() && Abaddon::Get().GetActiveChannelID() == message.ChannelID) return;
    // ignore if its our own message
    if (message.Author.ID == Abaddon::Get().GetDiscordClient().GetUserData().ID) return;
    // notify messages in DMs
    const auto channel = discord.GetChannel(message.ChannelID);
    if (channel->IsDM()) {
        NotifyMessageDM(message);
    } else if (CheckGuildMessage(message)) {
        NotifyMessageGuild(message);
    }
}

void Notifications::NotifyMessageDM(const Message &message) {
    Glib::ustring default_action = "app.go-to-channel";
    default_action += "::";
    default_action += std::to_string(message.ChannelID);
    const auto title = message.Author.Username;
    const auto body = message.Content;
    m_notifier.Notify(title, body, default_action);
}

void Notifications::NotifyMessageGuild(const Message &message) {
    Glib::ustring default_action = "app.go-to-channel";
    default_action += "::";
    default_action += std::to_string(message.ChannelID);
    Glib::ustring title = message.Author.Username;
    if (const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(message.ChannelID); channel.has_value() && channel->Name.has_value()) {
        if (channel->ParentID.has_value()) {
            const auto category = Abaddon::Get().GetDiscordClient().GetChannel(*channel->ParentID);
            if (category.has_value() && category->Name.has_value()) {
                title += " (#" + *channel->Name + ", " + *category->Name + ")";
            } else {
                title += " (#" + *channel->Name + ")";
            }
        } else {
            title += " (#" + *channel->Name + ")";
        }
    }
    const auto body = message.Content;
    m_notifier.Notify(title, body, default_action);
}

bool Notifications::IsDND() const {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto status = discord.GetUserStatus(discord.GetUserData().ID);
    return status == PresenceStatus::DND;
}
