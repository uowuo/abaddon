#include "pinnedwindow.hpp"
#include "../abaddon.hpp"

PinnedWindow::PinnedWindow(const ChannelData &data)
    : ChannelID(data.ID) {
    if (data.GuildID.has_value())
        GuildID = *data.GuildID;

    set_name("pinned-messages");
    set_default_size(450, 375);
    set_title("#" + *data.Name + " - Pinned Messages");
    set_position(Gtk::WIN_POS_CENTER);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");
    get_style_context()->add_class("pinned-messages-window");

    add(m_chat);
    m_chat.show();

    m_chat.SetSeparateAll(true);
    m_chat.SetActiveChannel(ChannelID);

    FetchPinned();
}

void PinnedWindow::FetchPinned() {
    Abaddon::Get().GetDiscordClient().FetchPinned(ChannelID, sigc::mem_fun(*this, &PinnedWindow::OnFetchedPinned));
}

void PinnedWindow::OnFetchedPinned(const std::vector<Message> &msgs, DiscordError code) {
    if (code != DiscordError::NONE) return;
    m_chat.SetMessages(msgs.begin(), msgs.end());
}
