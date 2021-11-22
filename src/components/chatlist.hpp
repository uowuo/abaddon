#pragma once
#include <gtkmm.h>
#include <map>
#include <vector>
#include "discord/snowflake.hpp"

class ChatList : public Gtk::ScrolledWindow {
public:
    ChatList();
    void Clear();
    void SetActiveChannel(Snowflake id);
    template<typename Iter>
    void SetMessages(Iter begin, Iter end);
    template<typename Iter>
    void PrependMessages(Iter begin, Iter end);
    void ProcessNewMessage(const Message &data, bool prepend);
    void DeleteMessage(Snowflake id);
    void RefetchMessage(Snowflake id);
    Snowflake GetOldestListedMessage();
    void UpdateMessageReactions(Snowflake id);
    void SetFailedByNonce(const std::string &nonce);
    std::vector<Snowflake> GetRecentAuthors();
    void SetSeparateAll(bool separate);
    void SetUsePinnedMenu();                  // i think i need a better way to do menus
    void ActuallyRemoveMessage(Snowflake id); // perhaps not the best method name

private:
    void OnScrollEdgeOvershot(Gtk::PositionType pos);
    void ScrollToBottom();
    void RemoveMessageAndHeader(Gtk::Widget *widget);

    bool m_use_pinned_menu = false;

    Gtk::Menu m_menu;
    Gtk::MenuItem *m_menu_copy_id;
    Gtk::MenuItem *m_menu_copy_content;
    Gtk::MenuItem *m_menu_delete_message;
    Gtk::MenuItem *m_menu_edit_message;
    Gtk::MenuItem *m_menu_reply_to;
    Gtk::MenuItem *m_menu_unpin;
    Gtk::MenuItem *m_menu_pin;
    Snowflake m_menu_selected_message;

    Snowflake m_active_channel;

    int m_num_messages = 0;
    int m_num_rows = 0;
    std::map<Snowflake, Gtk::Widget *> m_id_to_widget;

    bool m_should_scroll_to_bottom = true;
    Gtk::ListBox m_list;

    bool m_separate_all = false;

public:
    // these are all forwarded by the parent
    using type_signal_action_message_edit = sigc::signal<void, Snowflake, Snowflake>;
    using type_signal_action_chat_submit = sigc::signal<void, std::string, Snowflake, Snowflake>;
    using type_signal_action_chat_load_history = sigc::signal<void, Snowflake>;
    using type_signal_action_channel_click = sigc::signal<void, Snowflake>;
    using type_signal_action_insert_mention = sigc::signal<void, Snowflake>;
    using type_signal_action_open_user_menu = sigc::signal<void, const GdkEvent *, Snowflake, Snowflake>;
    using type_signal_action_reaction_add = sigc::signal<void, Snowflake, Glib::ustring>;
    using type_signal_action_reaction_remove = sigc::signal<void, Snowflake, Glib::ustring>;
    using type_signal_action_reply_to = sigc::signal<void, Snowflake>;

    type_signal_action_message_edit signal_action_message_edit();
    type_signal_action_chat_submit signal_action_chat_submit();
    type_signal_action_chat_load_history signal_action_chat_load_history();
    type_signal_action_channel_click signal_action_channel_click();
    type_signal_action_insert_mention signal_action_insert_mention();
    type_signal_action_open_user_menu signal_action_open_user_menu();
    type_signal_action_reaction_add signal_action_reaction_add();
    type_signal_action_reaction_remove signal_action_reaction_remove();
    type_signal_action_reply_to signal_action_reply_to();

private:
    type_signal_action_message_edit m_signal_action_message_edit;
    type_signal_action_chat_submit m_signal_action_chat_submit;
    type_signal_action_chat_load_history m_signal_action_chat_load_history;
    type_signal_action_channel_click m_signal_action_channel_click;
    type_signal_action_insert_mention m_signal_action_insert_mention;
    type_signal_action_open_user_menu m_signal_action_open_user_menu;
    type_signal_action_reaction_add m_signal_action_reaction_add;
    type_signal_action_reaction_remove m_signal_action_reaction_remove;
    type_signal_action_reply_to m_signal_action_reply_to;
};

template<typename Iter>
inline void ChatList::SetMessages(Iter begin, Iter end) {
    Clear();
    m_num_rows = 0;
    m_num_messages = 0;
    m_id_to_widget.clear();

    for (Iter it = begin; it != end; it++)
        ProcessNewMessage(*it, false);

    ScrollToBottom();
}

template<typename Iter>
inline void ChatList::PrependMessages(Iter begin, Iter end) {
    const auto old_upper = get_vadjustment()->get_upper();
    const auto old_value = get_vadjustment()->get_value();
    for (Iter it = begin; it != end; it++)
        ProcessNewMessage(*it, true);
    // force everything to process before getting new values
    while (Gtk::Main::events_pending())
        Gtk::Main::iteration();
    const auto new_upper = get_vadjustment()->get_upper();
    if (old_value == 0.0 && (new_upper - old_upper) > 0.0)
        get_vadjustment()->set_value(new_upper - old_upper);
    // this isn't ideal
}
