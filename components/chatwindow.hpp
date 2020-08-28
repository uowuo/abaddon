#pragma once
#include <gtkmm.h>
#include <queue>
#include <mutex>
#include "chatmessage.hpp"
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
    void AddNewHistory(const std::vector<MessageData> &msgs);
    void ClearMessages();

protected:
    void ScrollToBottom();
    void SetMessagesInternal();
    void AddNewMessageInternal();
    void AddNewHistoryInternal();
    ChatDisplayType GetMessageDisplayType(const MessageData *data);
    ChatMessageItem *CreateChatEntryComponentText(const MessageData *data);
    ChatMessageItem *CreateChatEntryComponent(const MessageData *data);
    void ProcessMessage(const MessageData *data, bool prepend = false);
    int m_num_rows = 0; // youd think thered be a Gtk::ListBox::get_row_count or something but nope

    bool m_scroll_to_bottom = true;

    bool on_key_press_event(GdkEventKey *e);
    void on_scroll_edge_overshot(Gtk::PositionType pos);

    Glib::Dispatcher m_message_set_dispatch;
    std::queue<std::unordered_set<const MessageData *>> m_message_set_queue;
    Glib::Dispatcher m_new_message_dispatch;
    std::queue<Snowflake> m_new_message_queue;
    Glib::Dispatcher m_new_history_dispatch;
    std::queue<std::vector<Snowflake>> m_new_history_queue;
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
