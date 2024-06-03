#pragma once

#include "discord/snowflake.hpp"

#include <vector>

#include <gtkmm/dialog.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchentry.h>

class QuickSwitcher : public Gtk::Dialog {
public:
    QuickSwitcher(Gtk::Window &parent);

private:
    void Index();
    void IndexPrivateChannels();
    void IndexChannels();
    void Search();

    void GoUp();
    void GoDown();
    void Move(int dir);

    void OnEntryActivate();
    bool OnEntryKeyPress(GdkEventKey *event);

    void OnResultRowActivate(Gtk::ListBoxRow *row);

    struct SwitcherEntry {
        Glib::ustring Name;
        uint64_t Sort; // lower = further up
        Snowflake ID;
    };
    std::vector<SwitcherEntry> m_index;

    Gtk::SearchEntry m_entry;
    Gtk::ScrolledWindow m_results_scroll;
    Gtk::ListBox m_results;
};
