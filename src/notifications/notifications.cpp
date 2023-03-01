#include "notifications.hpp"
#include "discord/message.hpp"

Notifications::Notifications() {
}

void Notifications::CheckMessage(const Message &message) {
    // ignore if our status is do not disturb
    if (IsDND()) return;
    auto &discord = Abaddon::Get().GetDiscordClient();
    // ignore if the channel is muted
    if (discord.IsChannelMuted(message.ChannelID)) return;
    // ignore if focused and message's channel is active
    if (Abaddon::Get().IsMainWindowActive() && Abaddon::Get().GetActiveChannelID() == message.ChannelID) return;
    // notify messages in DMs
    const auto channel = discord.GetChannel(message.ChannelID);
    if (channel->IsDM()) {
        NotifyMessage(message);
    }
}

void Notifications::NotifyMessage(const Message &message) {
    Glib::ustring default_action = "app.go-to-channel";
    default_action += "::";
    default_action += std::to_string(message.ChannelID);
    const auto title = message.Author.Username;
    const auto body = message.Content;
    m_notifier.Notify(title, body, default_action);
}

bool Notifications::IsDND() const {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto status = discord.GetUserStatus(discord.GetUserData().ID);
    return status == PresenceStatus::DND;
}
