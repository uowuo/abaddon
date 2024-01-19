#pragma once
// perhaps this should be conditionally included within cmakelists?
#ifdef WITH_LIBHANDY

// clang-format off

#include <unordered_map>
#include <gtkmm/box.h>
#include <handy.h>
#include "discord/snowflake.hpp"
#include "state.hpp"

// clang-format off

class ChannelData;

// thin wrapper over c api
// HdyTabBar + invisible HdyTabView since it needs one
class ChannelTabSwitcherHandy : public Gtk::Box {
public:
    ChannelTabSwitcherHandy();

    // no-op if already added
    void AddChannelTab(Snowflake id);
    // switches to existing tab if it exists
    void ReplaceActiveTab(Snowflake id);
    TabsState GetTabsState();
    void UseTabsState(const TabsState &state);

    void GoBackOnCurrent();
    void GoForwardOnCurrent();
    void GoToPreviousTab();
    void GoToNextTab();
    void GoToTab(int idx);

    [[nodiscard]] int GetNumberOfTabs() const;

private:
    void CheckUnread(Snowflake id);
    void ClearPage(HdyTabPage *page);
    void OnPageIconLoad(HdyTabPage *page, const Glib::RefPtr<Gdk::Pixbuf> &pb);
    void CheckPageIcon(HdyTabPage *page, const ChannelData &data);
    void AppendPageHistory(HdyTabPage *page, Snowflake channel);
    void AdvanceOnCurrent(size_t by);

    void OnChannelAccessibilityChanged(Snowflake id, bool accessibility);

    HdyTabBar *m_tab_bar;
    Gtk::Widget *m_tab_bar_wrapped;
    HdyTabView *m_tab_view;
    Gtk::Widget *m_tab_view_wrapped;

    std::unordered_map<Snowflake, HdyTabPage *> m_pages;
    std::unordered_map<HdyTabPage *, Snowflake> m_pages_rev;
    // need to hold a reference to the pixbuf data
    std::unordered_map<HdyTabPage *, Glib::RefPtr<Gdk::Pixbuf>> m_page_icons;

    struct PageHistory {
        std::vector<Snowflake> Visited;
        size_t CurrentVisitedIndex;
    };

    std::unordered_map<HdyTabPage *, PageHistory> m_page_history;

    friend void selected_page_notify_cb(HdyTabView *, GParamSpec *, ChannelTabSwitcherHandy *);
    friend gboolean close_page_cb(HdyTabView *, HdyTabPage *, ChannelTabSwitcherHandy *);

public:
    using type_signal_channel_switched_to = sigc::signal<void, Snowflake>;

    type_signal_channel_switched_to signal_channel_switched_to();

private:
    type_signal_channel_switched_to m_signal_channel_switched_to;
};
#endif
