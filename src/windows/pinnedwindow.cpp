#include "pinnedwindow.hpp"

#include "abaddon.hpp"

PinnedWindow::PinnedWindow(const ChannelData &data)
    : ChannelID(data.ID) {
    if (data.GuildID.has_value())
        GuildID = *data.GuildID;

    set_name("pinned-messages");
    set_default_size(450, 375);
    if (data.Name.has_value())
        set_title("#" + *data.Name + " - Pinned Messages");
    else
        set_title("Pinned Messages");
    set_position(Gtk::WIN_POS_CENTER);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");
    get_style_context()->add_class("pinned-messages-window");

    add(m_chat);
    m_chat.show();

    m_chat.SetSeparateAll(true);
    m_chat.SetActiveChannel(ChannelID);
    m_chat.SetUsePinnedMenu();

    Abaddon::Get().GetDiscordClient().signal_message_pinned().connect(sigc::mem_fun(*this, &PinnedWindow::OnMessagePinned));
    Abaddon::Get().GetDiscordClient().signal_message_unpinned().connect(sigc::mem_fun(*this, &PinnedWindow::OnMessageUnpinned));
    FetchPinned();
}

void PinnedWindow::OnMessagePinned(const Message &msg) {
    FetchPinned();
}

void PinnedWindow::OnMessageUnpinned(const Message &msg) {
    m_chat.ActuallyRemoveMessage(msg.ID);
}

void PinnedWindow::FetchPinned() {
    Abaddon::Get().GetDiscordClient().FetchPinned(ChannelID, sigc::mem_fun(*this, &PinnedWindow::OnFetchedPinned));
}

void PinnedWindow::OnFetchedPinned(const std::vector<Message> &msgs, DiscordError code) {
    if (code != DiscordError::NONE) return;
    m_chat.SetMessages(msgs.begin(), msgs.end());
}
