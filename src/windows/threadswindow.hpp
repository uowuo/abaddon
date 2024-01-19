#pragma once

#include <gtkmm/box.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/stack.h>
#include <gtkmm/stackswitcher.h>
#include <gtkmm/window.h>

#include "discord/objects.hpp"

class ActiveThreadsList : public Gtk::ScrolledWindow {
public:
    ActiveThreadsList(const ChannelData &channel, const Gtk::ListBox::SlotFilter &filter);

    void InvalidateFilter();

private:
    Gtk::ListBox m_list;

    using type_signal_thread_open = sigc::signal<void, Snowflake>;
    type_signal_thread_open m_signal_thread_open;

public:
    type_signal_thread_open signal_thread_open();
};

class ArchivedThreadsList : public Gtk::ScrolledWindow {
public:
    ArchivedThreadsList(const ChannelData &channel, const Gtk::ListBox::SlotFilter &filter);

    void InvalidateFilter();

private:
    Gtk::ListBox m_list;

    void OnThreadsFetched(DiscordError code, const ArchivedThreadsResponseData &data);

    using type_signal_thread_open = sigc::signal<void, Snowflake>;
    type_signal_thread_open m_signal_thread_open;

public:
    type_signal_thread_open signal_thread_open();
};

// view all threads in a channel
class ThreadsWindow : public Gtk::Window {
public:
    ThreadsWindow(const ChannelData &channel);

private:
    // this filtering is rather cringe but idk what a better alternative would be
    bool ListFilterFunc(Gtk::ListBoxRow *row_) const;

    enum FilterMode {
        FILTER_PUBLIC = 0,
        FILTER_PRIVATE = 1,
    };
    bool m_filter_mode = FILTER_PUBLIC;

    Snowflake m_channel_id;

    Gtk::StackSwitcher m_switcher;
    Gtk::Stack m_stack;

    Gtk::RadioButtonGroup m_group;
    Gtk::ButtonBox m_filter_buttons;
    Gtk::RadioButton m_filter_public;
    Gtk::RadioButton m_filter_private;

    Gtk::Box m_box;

    ActiveThreadsList m_active;
    ArchivedThreadsList m_archived;
};

class ThreadListRow : public Gtk::ListBoxRow {
public:
    ThreadListRow(const ChannelData &channel);

    Snowflake ID;
    ChannelType Type;

private:
    Gtk::Label m_label;
};
