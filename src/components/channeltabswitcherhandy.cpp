#ifdef WITH_LIBHANDY

    #include "channeltabswitcherhandy.hpp"
    #include "abaddon.hpp"

void selected_page_notify_cb(HdyTabView *view, GParamSpec *pspec, ChannelTabSwitcherHandy *switcher) {
    auto *page = hdy_tab_view_get_selected_page(view);
    if (auto it = switcher->m_pages_rev.find(page); it != switcher->m_pages_rev.end()) {
        switcher->m_signal_channel_switched_to.emit(it->second);
    }
}

gboolean close_page_cb(HdyTabView *view, HdyTabPage *page, ChannelTabSwitcherHandy *switcher) {
    switcher->ClearPage(page);
    hdy_tab_view_close_page_finish(view, page, true);
    return GDK_EVENT_STOP;
}

ChannelTabSwitcherHandy::ChannelTabSwitcherHandy() {
    m_tab_bar = hdy_tab_bar_new();
    m_tab_bar_wrapped = Glib::wrap(GTK_WIDGET(m_tab_bar));
    m_tab_view = hdy_tab_view_new();
    m_tab_view_wrapped = Glib::wrap(GTK_WIDGET(m_tab_view));

    g_signal_connect(m_tab_view, "notify::selected-page", G_CALLBACK(selected_page_notify_cb), this);
    g_signal_connect(m_tab_view, "close-page", G_CALLBACK(close_page_cb), this);

    hdy_tab_bar_set_view(m_tab_bar, m_tab_view);
    add(*m_tab_bar_wrapped);
    m_tab_bar_wrapped->show();

    auto &discord = Abaddon::Get().GetDiscordClient();
    discord.signal_message_create().connect([this](const Message &data) {
        CheckUnread(data.ChannelID);
    });

    discord.signal_message_ack().connect([this](const MessageAckData &data) {
        CheckUnread(data.ChannelID);
    });
}

void ChannelTabSwitcherHandy::AddChannelTab(Snowflake id) {
    if (m_pages.find(id) != m_pages.end()) return;

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto channel = discord.GetChannel(id);
    if (!channel.has_value()) return;

    auto *dummy = Gtk::make_managed<Gtk::Box>(); // minimal
    auto *page = hdy_tab_view_append(m_tab_view, GTK_WIDGET(dummy->gobj()));

    hdy_tab_page_set_title(page, channel->GetDisplayName().c_str());
    hdy_tab_page_set_tooltip(page, nullptr);

    m_pages[id] = page;
    m_pages_rev[page] = id;

    CheckUnread(id);
    CheckPageIcon(page, *channel);
}

void ChannelTabSwitcherHandy::ReplaceActiveTab(Snowflake id) {
    auto *page = hdy_tab_view_get_selected_page(m_tab_view);
    if (page == nullptr) {
        AddChannelTab(id);
    } else if (auto it = m_pages.find(id); it != m_pages.end()) {
        hdy_tab_view_set_selected_page(m_tab_view, it->second);
    } else {
        auto &discord = Abaddon::Get().GetDiscordClient();
        const auto channel = discord.GetChannel(id);
        if (!channel.has_value()) return;

        hdy_tab_page_set_title(page, channel->GetDisplayName().c_str());

        ClearPage(page);
        m_pages[id] = page;
        m_pages_rev[page] = id;

        CheckUnread(id);
        CheckPageIcon(page, *channel);
    }
}

void ChannelTabSwitcherHandy::CheckUnread(Snowflake id) {
    if (auto it = m_pages.find(id); it != m_pages.end()) {
        hdy_tab_page_set_needs_attention(it->second, Abaddon::Get().GetDiscordClient().GetUnreadStateForChannel(id) > -1);
    }
}

void ChannelTabSwitcherHandy::ClearPage(HdyTabPage *page) {
    if (auto it = m_pages_rev.find(page); it != m_pages_rev.end()) {
        m_pages.erase(it->second);
    }
    m_pages_rev.erase(page);
    m_page_icons.erase(page);
}

void ChannelTabSwitcherHandy::OnPageIconLoad(HdyTabPage *page, const Glib::RefPtr<Gdk::Pixbuf> &pb) {
    auto new_pb = pb->scale_simple(16, 16, Gdk::INTERP_BILINEAR);
    m_page_icons[page] = new_pb;
    hdy_tab_page_set_icon(page, G_ICON(new_pb->gobj()));
}

void ChannelTabSwitcherHandy::CheckPageIcon(HdyTabPage *page, const ChannelData &data) {
    if (data.GuildID.has_value()) {
        if (const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(*data.GuildID); guild.has_value() && guild->HasIcon()) {
            auto *child_widget = hdy_tab_page_get_child(page);
            if (child_widget == nullptr) return; // probably wont happen :---)
            // i think this works???
            auto *trackable = Glib::wrap(GTK_WIDGET(child_widget));

            Abaddon::Get().GetImageManager().LoadFromURL(
                guild->GetIconURL("png", "16"),
                sigc::track_obj([this, page](const Glib::RefPtr<Gdk::Pixbuf> &pb) { OnPageIconLoad(page, pb); },
                                *trackable));
            return;
        }
        return;
    }

    hdy_tab_page_set_icon(page, nullptr);
}

ChannelTabSwitcherHandy::type_signal_channel_switched_to ChannelTabSwitcherHandy::signal_channel_switched_to() {
    return m_signal_channel_switched_to;
}

#endif
