#ifdef WITH_LIBHANDY

// clang-format off

#include "channeltabswitcherhandy.hpp"
#include "abaddon.hpp"

// clang-format on

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

    m_tab_bar_wrapped->get_style_context()->add_class("channel-tab-switcher");

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

    discord.signal_channel_accessibility_changed().connect(sigc::mem_fun(*this, &ChannelTabSwitcherHandy::OnChannelAccessibilityChanged));
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
    AppendPageHistory(page, id);
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
        AppendPageHistory(page, id);
    }
}

TabsState ChannelTabSwitcherHandy::GetTabsState() {
    TabsState state;

    const gint num_pages = hdy_tab_view_get_n_pages(m_tab_view);
    for (gint i = 0; i < num_pages; i++) {
        auto *page = hdy_tab_view_get_nth_page(m_tab_view, i);
        if (page != nullptr) {
            if (const auto it = m_pages_rev.find(page); it != m_pages_rev.end()) {
                state.Channels.push_back(it->second);
            }
        }
    }

    return state;
}

void ChannelTabSwitcherHandy::UseTabsState(const TabsState &state) {
    for (auto id : state.Channels) {
        AddChannelTab(id);
    }
}

void ChannelTabSwitcherHandy::GoBackOnCurrent() {
    AdvanceOnCurrent(-1);
}

void ChannelTabSwitcherHandy::GoForwardOnCurrent() {
    AdvanceOnCurrent(1);
}

void ChannelTabSwitcherHandy::GoToPreviousTab() {
    if (!hdy_tab_view_select_previous_page(m_tab_view)) {
        if (const auto num_pages = hdy_tab_view_get_n_pages(m_tab_view); num_pages > 1) {
            hdy_tab_view_set_selected_page(m_tab_view, hdy_tab_view_get_nth_page(m_tab_view, num_pages - 1));
        }
    }
}

void ChannelTabSwitcherHandy::GoToNextTab() {
    if (!hdy_tab_view_select_next_page(m_tab_view)) {
        if (hdy_tab_view_get_n_pages(m_tab_view) > 1) {
            hdy_tab_view_set_selected_page(m_tab_view, hdy_tab_view_get_nth_page(m_tab_view, 0));
        }
    }
}

void ChannelTabSwitcherHandy::GoToTab(int idx) {
    if (hdy_tab_view_get_n_pages(m_tab_view) >= idx + 1)
        hdy_tab_view_set_selected_page(m_tab_view, hdy_tab_view_get_nth_page(m_tab_view, idx));
}

int ChannelTabSwitcherHandy::GetNumberOfTabs() const {
    return hdy_tab_view_get_n_pages(m_tab_view);
}

void ChannelTabSwitcherHandy::CheckUnread(Snowflake id) {
    if (auto it = m_pages.find(id); it != m_pages.end()) {
        auto &discord = Abaddon::Get().GetDiscordClient();
        const bool has_unreads = discord.GetUnreadStateForChannel(id) > -1;
        const bool show_indicator = has_unreads && !discord.IsChannelMuted(id);
        hdy_tab_page_set_needs_attention(it->second, show_indicator);
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
    std::optional<std::string> icon_url;

    if (data.GuildID.has_value()) {
        if (const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(*data.GuildID); guild.has_value() && guild->HasIcon()) {
            icon_url = guild->GetIconURL("png", "16");
        }
    } else if (data.IsDM()) {
        icon_url = data.GetIconURL();
    }

    if (icon_url.has_value()) {
        auto *child_widget = hdy_tab_page_get_child(page);
        if (child_widget == nullptr) return; // probably wont happen :---)
        // i think this works???
        auto *trackable = Glib::wrap(GTK_WIDGET(child_widget));
        Abaddon::Get().GetImageManager().LoadFromURL(
            *icon_url,
            sigc::track_obj([this, page](const Glib::RefPtr<Gdk::Pixbuf> &pb) { OnPageIconLoad(page, pb); },
                            *trackable));

        return;
    }

    hdy_tab_page_set_icon(page, nullptr);
}

void ChannelTabSwitcherHandy::AppendPageHistory(HdyTabPage *page, Snowflake channel) {
    auto it = m_page_history.find(page);
    if (it == m_page_history.end()) {
        m_page_history[page] = PageHistory { { channel }, 0 };
        return;
    }

    // drop everything beyond current position
    it->second.Visited.resize(++it->second.CurrentVisitedIndex);
    it->second.Visited.push_back(channel);
}

void ChannelTabSwitcherHandy::AdvanceOnCurrent(size_t by) {
    auto *current = hdy_tab_view_get_selected_page(m_tab_view);
    if (current == nullptr) return;
    auto history = m_page_history.find(current);
    if (history == m_page_history.end()) return;
    if (by + history->second.CurrentVisitedIndex < 0 || by + history->second.CurrentVisitedIndex >= history->second.Visited.size()) return;

    history->second.CurrentVisitedIndex += by;
    const auto to_id = history->second.Visited.at(history->second.CurrentVisitedIndex);

    // temporarily point current index to the end so that it doesnt fuck up the history
    // remove it immediately after cuz the emit will call ReplaceActiveTab
    const auto real = history->second.CurrentVisitedIndex;
    history->second.CurrentVisitedIndex = history->second.Visited.size() - 1;
    m_signal_channel_switched_to.emit(to_id);
    // iterator might not be valid
    history = m_page_history.find(current);
    if (history != m_page_history.end()) {
        history->second.Visited.pop_back();
    }
    history->second.CurrentVisitedIndex = real;
}

void ChannelTabSwitcherHandy::OnChannelAccessibilityChanged(Snowflake id, bool accessibility) {
    if (accessibility) return;
    if (auto it = m_pages.find(id); it != m_pages.end()) {
        if (hdy_tab_page_get_selected(it->second))
            if (!hdy_tab_view_select_previous_page(m_tab_view))
                hdy_tab_view_select_next_page(m_tab_view);

        hdy_tab_view_close_page(m_tab_view, it->second);
        ClearPage(it->second);
    }
}

ChannelTabSwitcherHandy::type_signal_channel_switched_to ChannelTabSwitcherHandy::signal_channel_switched_to() {
    return m_signal_channel_switched_to;
}

#endif
