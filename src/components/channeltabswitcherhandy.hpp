#pragma once
// perhaps this should be conditionally included within cmakelists?
#ifdef WITH_LIBHANDY
    #include <gtkmm/box.h>
    #include <unordered_map>
    #include <handy.h>
    #include "discord/snowflake.hpp"

// thin wrapper over c api
// HdyTabBar + invisible HdyTabView since it needs one
class ChannelTabSwitcherHandy : public Gtk::Box {
public:
    ChannelTabSwitcherHandy();

    void AddChannelTab(Snowflake id);
    void ReplaceActiveTab(Snowflake id);

private:
    HdyTabBar *m_tab_bar;
    Gtk::Widget *m_tab_bar_wrapped;
    HdyTabView *m_tab_view;
    Gtk::Widget *m_tab_view_wrapped;

    std::unordered_map<Snowflake, HdyTabPage *> m_pages;
    std::unordered_map<HdyTabPage *, Snowflake> m_pages_rev;

    friend void selected_page_notify_cb(HdyTabView *, GParamSpec *, ChannelTabSwitcherHandy *);

public:
    using type_signal_channel_switched_to = sigc::signal<void, Snowflake>;

    type_signal_channel_switched_to signal_channel_switched_to();

private:
    type_signal_channel_switched_to m_signal_channel_switched_to;
};
#endif
