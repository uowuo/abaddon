#ifdef WITH_LIBHANDY

    #include "channeltabswitcherhandy.hpp"
    #include "abaddon.hpp"

void selected_page_notify_cb(HdyTabView *view, GParamSpec *pspec, ChannelTabSwitcherHandy *switcher) {
    auto *page = hdy_tab_view_get_selected_page(view);
    if (auto it = switcher->m_pages_rev.find(page); it != switcher->m_pages_rev.end()) {
        switcher->m_signal_channel_switched_to.emit(it->second);
    }
}

ChannelTabSwitcherHandy::ChannelTabSwitcherHandy() {
    m_tab_bar = hdy_tab_bar_new();
    m_tab_bar_wrapped = Glib::wrap(GTK_WIDGET(m_tab_bar));
    m_tab_view = hdy_tab_view_new();
    m_tab_view_wrapped = Glib::wrap(GTK_WIDGET(m_tab_view));

    g_signal_connect(m_tab_view, "notify::selected-page", G_CALLBACK(selected_page_notify_cb), this);

    hdy_tab_bar_set_view(m_tab_bar, m_tab_view);
    add(*m_tab_bar_wrapped);
    m_tab_bar_wrapped->show();
}

void ChannelTabSwitcherHandy::AddChannelTab(Snowflake id) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto channel = discord.GetChannel(id);
    if (!channel.has_value()) return;

    auto *dummy = Gtk::make_managed<Gtk::Box>(); // minimal
    auto *page = hdy_tab_view_append(m_tab_view, GTK_WIDGET(dummy->gobj()));

    hdy_tab_page_set_title(page, ("#" + *channel->Name).c_str());

    m_pages[id] = page;
    m_pages_rev[page] = id;
}

void ChannelTabSwitcherHandy::ReplaceActiveTab(Snowflake id) {
    auto *page = hdy_tab_view_get_selected_page(m_tab_view);
    if (page == nullptr) {
        AddChannelTab(id);
    } else {
        auto &discord = Abaddon::Get().GetDiscordClient();
        const auto channel = discord.GetChannel(id);
        if (!channel.has_value()) return;

        hdy_tab_page_set_title(page, ("#" + *channel->Name).c_str());
        m_pages_rev[page] = id;
    }
}

ChannelTabSwitcherHandy::type_signal_channel_switched_to ChannelTabSwitcherHandy::signal_channel_switched_to() {
    return m_signal_channel_switched_to;
}

#endif
