#pragma once
#include <gtkmm.h>
#include <string>
#include <sigc++/sigc++.h>
#include "../discord/discord.hpp"

enum class ChatDisplayType {
    Unknown,
    Text,
    Embed,
};

// contains the username and timestamp, chat items get stuck into its box
class ChatMessageContainer : public Gtk::ListBoxRow {
public:
    Snowflake UserID;
    Snowflake ChannelID;

    ChatMessageContainer(const MessageData *data);
    void AddNewContent(Gtk::Widget *widget, bool prepend = false);
    void Update();

protected:
    Gtk::Box *m_main_box;
    Gtk::Box *m_content_box;
    Gtk::Box *m_meta_box;
    Gtk::Label *m_author;
    Gtk::Label *m_timestamp;
};

class ChatMessageItem {
public:
    ChatMessageItem();

    Snowflake ChannelID;
    Snowflake ID;
    ChatDisplayType MessageType = ChatDisplayType::Unknown;

    virtual void ShowMenu(const GdkEvent *event);
    void AddMenuItem(Gtk::MenuItem *item);
    virtual void MarkAsDeleted() = 0;
    virtual void MarkAsEdited() = 0;

protected:
    void AttachMenuHandler(Gtk::Widget *widget);
    void on_menu_copy_id();
    void on_menu_message_delete();
    void on_menu_message_edit();

    Gtk::Menu m_menu;
    Gtk::MenuItem *m_menu_copy_id;
    Gtk::MenuItem *m_menu_delete_message;
    Gtk::MenuItem *m_menu_edit_message;

public:
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_message_delete;
    typedef sigc::signal<void, Snowflake, Snowflake> type_signal_action_message_edit;

    type_signal_action_message_delete signal_action_message_delete();
    type_signal_action_message_edit signal_action_message_edit();

private:
    type_signal_action_message_delete m_signal_action_message_delete;
    type_signal_action_message_edit m_signal_action_message_edit;
};

class ChatMessageTextItem
    : public Gtk::TextView // oh well
    , public ChatMessageItem {
public:
    ChatMessageTextItem(const MessageData *data);

    void EditContent(std::string content);

    virtual void MarkAsDeleted();
    virtual void MarkAsEdited();

protected:
    void UpdateAttributes();

    std::string m_content;

    bool m_was_deleted = false;
    bool m_was_edited = false;

    void on_menu_copy_content();
    Gtk::MenuItem *m_menu_copy_content;
    Gtk::MenuItem *m_menu_delete_message;
};

class ChatMessageEmbedItem
    : public Gtk::EventBox
    , public ChatMessageItem {
public:
    ChatMessageEmbedItem(const MessageData *data);

    virtual void MarkAsDeleted();
    virtual void MarkAsEdited();

protected:
    void DoLayout();
    void UpdateAttributes();

    bool m_was_deleted = false;
    bool m_was_edited = false;

    EmbedData m_embed;
    Gtk::Box *m_main;
    Gtk::Label *m_attrib_label = nullptr;
};
