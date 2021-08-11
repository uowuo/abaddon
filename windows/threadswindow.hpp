#pragma once
#include <gtkmm.h>
#include "../discord/objects.hpp"

class ActiveThreadsList : public Gtk::ScrolledWindow {
public:
    ActiveThreadsList(const ChannelData &channel);

private:
    Gtk::ListBox m_list;

    using type_signal_thread_open = sigc::signal<void, Snowflake>;
    type_signal_thread_open m_signal_thread_open;

public:
    type_signal_thread_open signal_thread_open();
};

class ArchivedThreadsList : public Gtk::ScrolledWindow {
public:
    ArchivedThreadsList(const ChannelData &channel);

private:
    Gtk::ListBox m_list;

    void OnPublicFetched(DiscordError code, const ArchivedThreadsResponseData &data);

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
    Snowflake m_channel_id;

    Gtk::StackSwitcher m_switcher;
    Gtk::Stack m_stack;

    Gtk::Box m_box;

    ActiveThreadsList m_active;
    ArchivedThreadsList m_archived;
};

class ThreadListRow : public Gtk::ListBoxRow {
public:
    ThreadListRow(const ChannelData &channel);

    Snowflake ID;

private:
    Gtk::Label m_label;
};
