#pragma once
#include <gtkmm.h>
#include "../discord/errors.hpp"
#include "../discord/channel.hpp"
#include "../discord/message.hpp"
#include "../components/chatlist.hpp"

class PinnedWindow : public Gtk::Window {
public:
    PinnedWindow(const ChannelData &data);

    Snowflake GuildID;
    Snowflake ChannelID;

private:
    void FetchPinned();
    void OnFetchedPinned(const std::vector<Message> &msgs, DiscordError code);

    ChatList m_chat;
};
