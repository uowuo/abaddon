#pragma once
#include <gtkmm.h>
#include <queue>
#include <mutex>
#include "../discord/discord.hpp"

class ChatWindow {
public:
    ChatWindow();
    Gtk::Widget *GetRoot() const;
    void SetActiveChannel(Snowflake id);
    Snowflake GetActiveChannel() const;
    void SetMessages(std::unordered_set<const MessageData *> msgs);

protected:
    void ScrollToBottom();
    void SetMessagesInternal();
    Gtk::ListBoxRow *CreateChatEntryComponentText(const MessageData *data);
    Gtk::ListBoxRow *CreateChatEntryComponent(const MessageData *data);

    Glib::Dispatcher m_update_dispatcher;
    std::queue<std::unordered_set<const MessageData *>> m_update_queue;
    std::mutex m_update_mutex;

    Snowflake m_active_channel;

    Gtk::Box *m_main;
    Gtk::ListBox *m_listbox;
    Gtk::Viewport *m_viewport;
    Gtk::ScrolledWindow *m_scroll;
    Gtk::ScrolledWindow *m_entry_scroll;
    Gtk::TextView *m_input;
};
