#pragma once
#include <gtkmm.h>
#include "../discord/discord.hpp"

enum class ChatDisplayType {
    Unknown,
    Text,
};

class Abaddon;

// contains the username and timestamp, chat items get stuck into its box
class ChatMessageContainer : public Gtk::ListBoxRow {
public:
    Snowflake UserID;

    ChatMessageContainer(const MessageData *data);
    void AddNewContent(Gtk::Widget *widget, bool prepend = false);

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
    void SetAbaddon(Abaddon *ptr);

    Snowflake ChannelID;
    Snowflake ID;
    ChatDisplayType MessageType = ChatDisplayType::Unknown;

    virtual void ShowMenu(const GdkEvent *event);
    void AddMenuItem(Gtk::MenuItem *item);
    virtual void MarkAsDeleted() = 0;

protected:
    void AttachMenuHandler(Gtk::Widget *widget);
    void on_menu_copy_id();
    void on_menu_message_delete();

    Gtk::Menu m_menu;
    Gtk::MenuItem *m_menu_copy_id;
    Gtk::MenuItem *m_menu_delete_message;

    Abaddon *m_abaddon = nullptr;
};

class ChatMessageTextItem
    : public Gtk::TextView // oh well
    , public ChatMessageItem {
public:
    ChatMessageTextItem(const MessageData *data);
    virtual void MarkAsDeleted();

protected:
    void on_menu_copy_content();
    Gtk::MenuItem *m_menu_copy_content;
    Gtk::MenuItem *m_menu_delete_message;
};
