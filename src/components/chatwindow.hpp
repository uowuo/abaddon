#pragma once
#include <gtkmm.h>
#include <string>
#include <set>
#include "discord/discord.hpp"
#include "completer.hpp"

class ChatMessageHeader;
class ChatMessageItemContainer;
class ChatInput;
class ChatInputIndicator;
class RateLimitIndicator;
class ChatList;
class ChatWindow {
public:
    ChatWindow();

    Gtk::Widget *GetRoot() const;
    Snowflake GetActiveChannel() const;

    void Clear();
    void SetMessages(const std::vector<Message> &msgs); // clear contents and replace with given set
    void SetActiveChannel(Snowflake id);
    void AddNewMessage(const Message &data);              // append new message to bottom
    void DeleteMessage(Snowflake id);                     // add [deleted] indicator
    void UpdateMessage(Snowflake id);                     // add [edited] indicator
    void AddNewHistory(const std::vector<Message> &msgs); // prepend messages
    void InsertChatInput(std::string text);
    Snowflake GetOldestListedMessage(); // oldest message that is currently in the ListBox
    void UpdateReactions(Snowflake id);
    void SetTopic(const std::string &text);

protected:
    bool m_is_replying = false;
    Snowflake m_replying_to;

    void StartReplying(Snowflake message_id);
    void StopReplying();

    Snowflake m_active_channel;

    bool OnInputSubmit(const Glib::ustring &text);

    bool OnKeyPressEvent(GdkEventKey *e);
    void OnScrollEdgeOvershot(Gtk::PositionType pos);

    void OnMessageSendFail(const std::string &nonce, float retry_after);

    Gtk::Box *m_main;
    //Gtk::ListBox *m_list;
    //Gtk::ScrolledWindow *m_scroll;

    Gtk::EventBox m_topic; // todo probably make everything else go on the stack
    Gtk::Label m_topic_text;

    ChatList *m_chat;

    ChatInput *m_input;

    Completer m_completer;
    ChatInputIndicator *m_input_indicator;
    RateLimitIndicator *m_rate_limit_indicator;
    Gtk::Box *m_meta;

public:
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_message_edit;
    typedef sigc::signal<void, std::string, Snowflake, Snowflake> type_signal_action_chat_submit;
    typedef sigc::signal<void, Snowflake> type_signal_action_chat_load_history;
    typedef sigc::signal<void, Snowflake> type_signal_action_channel_click;
    typedef sigc::signal<void, Snowflake> type_signal_action_insert_mention;
    typedef sigc::signal<void, Snowflake, Glib::ustring> type_signal_action_reaction_add;
    typedef sigc::signal<void, Snowflake, Glib::ustring> type_signal_action_reaction_remove;

    type_signal_action_message_edit signal_action_message_edit();
    type_signal_action_chat_submit signal_action_chat_submit();
    type_signal_action_chat_load_history signal_action_chat_load_history();
    type_signal_action_channel_click signal_action_channel_click();
    type_signal_action_insert_mention signal_action_insert_mention();
    type_signal_action_reaction_add signal_action_reaction_add();
    type_signal_action_reaction_remove signal_action_reaction_remove();

private:
    type_signal_action_message_edit m_signal_action_message_edit;
    type_signal_action_chat_submit m_signal_action_chat_submit;
    type_signal_action_chat_load_history m_signal_action_chat_load_history;
    type_signal_action_channel_click m_signal_action_channel_click;
    type_signal_action_insert_mention m_signal_action_insert_mention;
    type_signal_action_reaction_add m_signal_action_reaction_add;
    type_signal_action_reaction_remove m_signal_action_reaction_remove;
};
