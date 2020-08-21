#pragma once
#include <gtkmm.h>
#include <queue>
#include <mutex>
#include "../discord/discord.hpp"

class Abaddon;
class ChatWindow {
public:
    ChatWindow();
    void SetAbaddon(Abaddon *ptr);

    Gtk::Widget *GetRoot() const;
    void SetActiveChannel(Snowflake id);
    Snowflake GetActiveChannel() const;
    void SetMessages(std::unordered_set<const MessageData *> msgs);
    void AddNewMessage(Snowflake id);

protected:
    void ScrollToBottom();
    void SetMessagesInternal();
    void AddNewMessageInternal();
    Gtk::ListBoxRow *CreateChatEntryComponentText(const MessageData *data);
    Gtk::ListBoxRow *CreateChatEntryComponent(const MessageData *data);

    bool on_key_press_event(GdkEventKey *e);

    Glib::Dispatcher m_message_set_dispatch;
    std::queue<std::unordered_set<const MessageData *>> m_message_set_queue;
    Glib::Dispatcher m_new_message_dispatch;
    std::queue<Snowflake> m_new_message_queue;
    std::mutex m_update_mutex;

    Snowflake m_active_channel;

    Gtk::Box *m_main;
    Gtk::ListBox *m_listbox;
    Gtk::Viewport *m_viewport;
    Gtk::ScrolledWindow *m_scroll;
    Gtk::ScrolledWindow *m_entry_scroll;
    Gtk::TextView *m_input;

    Abaddon *m_abaddon = nullptr;
};
