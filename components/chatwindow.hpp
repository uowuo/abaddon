#pragma once
#include <gtkmm.h>
#include <string>
#include <set>
#include "../discord/discord.hpp"
#include "chatmessage.hpp"
#include "completer.hpp"

class ChatInput;
class ChatInputIndicator;
class ChatWindow {
public:
    ChatWindow();

    Gtk::Widget *GetRoot() const;
    Snowflake GetActiveChannel() const;

    void Clear();
    void SetMessages(const std::set<Snowflake> &msgs); // clear contents and replace with given set
    void SetActiveChannel(Snowflake id);
    void AddNewMessage(Snowflake id);                     // append new message to bottom
    void DeleteMessage(Snowflake id);                     // add [deleted] indicator
    void UpdateMessage(Snowflake id);                     // add [edited] indicator
    void AddNewHistory(const std::vector<Snowflake> &id); // prepend messages
    void InsertChatInput(std::string text);
    Snowflake GetOldestListedMessage(); // oldest message that is currently in the ListBox
    void UpdateReactions(Snowflake id);

protected:
    ChatMessageItemContainer *CreateMessageComponent(Snowflake id); // to be inserted into header's content box
    void ProcessNewMessage(Snowflake id, bool prepend);             // creates and adds components

    bool m_is_replying = false;
    Snowflake m_replying_to;

    void StartReplying(Snowflake message_id);
    void StopReplying();

    int m_num_messages = 0;
    int m_num_rows = 0;
    std::map<Snowflake, Gtk::Widget *> m_id_to_widget;

    Snowflake m_active_channel;

    void OnInputSubmit(const Glib::ustring &text);

    bool OnKeyPressEvent(GdkEventKey *e);
    void OnScrollEdgeOvershot(Gtk::PositionType pos);

    void RemoveMessageAndHeader(Gtk::Widget *widget);

    void ScrollToBottom();
    bool m_should_scroll_to_bottom = true;

    void OnMessageSendFail(const std::string &nonce);

    Gtk::Box *m_main;
    Gtk::ListBox *m_list;
    Gtk::ScrolledWindow *m_scroll;

    ChatInput *m_input;

    Completer m_completer;
    ChatInputIndicator *m_input_indicator;

public:
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_message_delete;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_message_edit;
    typedef sigc::signal<void, std::string, Snowflake, Snowflake> type_signal_action_chat_submit;
    typedef sigc::signal<void, Snowflake> type_signal_action_chat_load_history;
    typedef sigc::signal<void, Snowflake> type_signal_action_channel_click;
    typedef sigc::signal<void, Snowflake> type_signal_action_insert_mention;
    typedef sigc::signal<void, const GdkEvent *, Snowflake, Snowflake> type_signal_action_open_user_menu;
    typedef sigc::signal<void, Snowflake, Glib::ustring> type_signal_action_reaction_add;
    typedef sigc::signal<void, Snowflake, Glib::ustring> type_signal_action_reaction_remove;

    type_signal_action_message_delete signal_action_message_delete();
    type_signal_action_message_edit signal_action_message_edit();
    type_signal_action_chat_submit signal_action_chat_submit();
    type_signal_action_chat_load_history signal_action_chat_load_history();
    type_signal_action_channel_click signal_action_channel_click();
    type_signal_action_insert_mention signal_action_insert_mention();
    type_signal_action_open_user_menu signal_action_open_user_menu();
    type_signal_action_reaction_add signal_action_reaction_add();
    type_signal_action_reaction_remove signal_action_reaction_remove();

private:
    type_signal_action_message_delete m_signal_action_message_delete;
    type_signal_action_message_edit m_signal_action_message_edit;
    type_signal_action_chat_submit m_signal_action_chat_submit;
    type_signal_action_chat_load_history m_signal_action_chat_load_history;
    type_signal_action_channel_click m_signal_action_channel_click;
    type_signal_action_insert_mention m_signal_action_insert_mention;
    type_signal_action_open_user_menu m_signal_action_open_user_menu;
    type_signal_action_reaction_add m_signal_action_reaction_add;
    type_signal_action_reaction_remove m_signal_action_reaction_remove;
};
