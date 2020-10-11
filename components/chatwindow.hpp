#pragma once
#include <gtkmm.h>
#include <string>
#include <mutex>
#include <queue>
#include <set>
#include "../discord/discord.hpp"
#include "chatmessage.hpp"

class ChatWindow {
public:
    ChatWindow();

    Gtk::Widget *GetRoot() const;
    Snowflake GetActiveChannel() const;

    void Clear();
    void SetMessages(const std::set<Snowflake> &msgs); // clear contents and replace with given set
    void SetActiveChannel(Snowflake id);
    void AddNewMessage(Snowflake id); // append new message to bottom
    void DeleteMessage(Snowflake id); // add [deleted] indicator
    void UpdateMessage(Snowflake id); // add [edited] indicator
    void AddNewHistory(const std::vector<Snowflake> &id); // prepend messages
    void InsertChatInput(std::string text);
    Snowflake GetOldestListedMessage(); // oldest message that is currently in the ListBox

protected:
    ChatMessageItemContainer *CreateMessageComponent(Snowflake id); // to be inserted into header's content box
    void ProcessNewMessage(Snowflake id, bool prepend); // creates and adds components

    void SetMessagesInternal();
    void AddNewMessageInternal();
    void DeleteMessageInternal();
    void UpdateMessageInternal();
    void AddNewHistoryInternal();

    int m_num_rows = 0;
    std::unordered_map<Snowflake, Gtk::Widget *> m_id_to_widget;

    Glib::Dispatcher m_set_messsages_dispatch;
    std::queue<std::set<Snowflake>> m_set_messages_queue;
    Glib::Dispatcher m_new_message_dispatch;
    std::queue<Snowflake> m_new_message_queue;
    Glib::Dispatcher m_delete_message_dispatch;
    std::queue<Snowflake> m_delete_message_queue;
    Glib::Dispatcher m_update_message_dispatch;
    std::queue<Snowflake> m_update_message_queue;
    Glib::Dispatcher m_history_dispatch;
    std::queue<std::set<Snowflake>> m_history_queue;
    mutable std::mutex m_update_mutex;

    Snowflake m_active_channel;

    bool on_key_press_event(GdkEventKey *e);
    void on_scroll_edge_overshot(Gtk::PositionType pos);

    void ScrollToBottom();
    bool m_should_scroll_to_bottom = true;

    Gtk::Box *m_main;
    Gtk::ListBox *m_list;
    Gtk::ScrolledWindow *m_scroll;

    Gtk::TextView *m_input;
    Gtk::ScrolledWindow *m_input_scroll;

public:
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_message_delete;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_message_edit;
    typedef sigc::signal<void, std::string, Snowflake> type_signal_action_chat_submit;
    typedef sigc::signal<void, Snowflake> type_signal_action_chat_load_history;
    typedef sigc::signal<void, Snowflake> type_signal_action_channel_click;

    type_signal_action_message_delete signal_action_message_delete();
    type_signal_action_message_edit signal_action_message_edit();
    type_signal_action_chat_submit signal_action_chat_submit();
    type_signal_action_chat_load_history signal_action_chat_load_history();
    type_signal_action_channel_click signal_action_channel_click();

private:
    type_signal_action_message_delete m_signal_action_message_delete;
    type_signal_action_message_edit m_signal_action_message_edit;
    type_signal_action_chat_submit m_signal_action_chat_submit;
    type_signal_action_chat_load_history m_signal_action_chat_load_history;
    type_signal_action_channel_click m_signal_action_channel_click;
};
