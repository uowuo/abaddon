#pragma once
#include <gtkmm.h>
#include <queue>
#include <mutex>
#include <unordered_map>
#include <sigc++/sigc++.h>
#include "chatmessage.hpp"
#include "../discord/discord.hpp"

class ChatWindow {
public:
    ChatWindow();

    Gtk::Widget *GetRoot() const;
    void SetActiveChannel(Snowflake id);
    Snowflake GetActiveChannel() const;
    void SetMessages(std::set<Snowflake> msgs);
    void AddNewMessage(Snowflake id);
    void AddNewHistory(const std::vector<Snowflake> &msgs);
    void DeleteMessage(Snowflake id);
    void UpdateMessageContent(Snowflake id);
    void Clear();
    void InsertChatInput(std::string text);
    Snowflake GetOldestListedMessage();

protected:
    void ScrollToBottom();
    void SetMessagesInternal();
    void AddNewMessageInternal();
    void AddNewHistoryInternal();
    void DeleteMessageInternal();
    void UpdateMessageContentInternal();
    ChatDisplayType GetMessageDisplayType(const Message *data);
    void ProcessMessage(const Message *data, bool prepend = false);
    int m_num_rows = 0; // youd think thered be a Gtk::ListBox::get_row_count or something but nope
    std::unordered_map<Snowflake, ChatMessageItem *> m_id_to_widget;

    bool m_scroll_to_bottom = true;

    bool on_key_press_event(GdkEventKey *e);
    void on_scroll_edge_overshot(Gtk::PositionType pos);

    Glib::Dispatcher m_message_set_dispatch;
    std::queue<std::set<Snowflake>> m_message_set_queue;
    Glib::Dispatcher m_new_message_dispatch;
    std::queue<Snowflake> m_new_message_queue;
    Glib::Dispatcher m_new_history_dispatch;
    std::queue<std::vector<Snowflake>> m_new_history_queue;
    Glib::Dispatcher m_message_delete_dispatch;
    std::queue<Snowflake> m_message_delete_queue;
    Glib::Dispatcher m_message_edit_dispatch;
    std::queue<Snowflake> m_message_edit_queue;
    std::mutex m_update_mutex;

    Snowflake m_active_channel;

    Gtk::Box *m_main;
    Gtk::ListBox *m_listbox;
    Gtk::Viewport *m_viewport;
    Gtk::ScrolledWindow *m_scroll;
    Gtk::ScrolledWindow *m_entry_scroll;
    Gtk::TextView *m_input;

public:
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_message_delete;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_message_edit;
    typedef sigc::signal<void, std::string, Snowflake> type_signal_action_chat_submit;
    typedef sigc::signal<void, Snowflake> type_signal_action_chat_load_history;

    type_signal_action_message_delete signal_action_message_delete();
    type_signal_action_message_edit signal_action_message_edit();
    type_signal_action_chat_submit signal_action_chat_submit();
    type_signal_action_chat_load_history signal_action_chat_load_history();

private:
    type_signal_action_message_delete m_signal_action_message_delete;
    type_signal_action_message_edit m_signal_action_message_edit;
    type_signal_action_chat_submit m_signal_action_chat_submit;
    type_signal_action_chat_load_history m_signal_action_chat_load_history;
};
