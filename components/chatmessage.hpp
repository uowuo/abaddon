#pragma once
#include <gtkmm.h>
#include "../discord/discord.hpp"

enum class ChatDisplayType {
    Unknown,
    Text,
};

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
    Snowflake ID;
    ChatDisplayType MessageType;

    virtual void MarkAsDeleted() = 0;
};

class ChatMessageTextItem
    : public Gtk::TextView // oh well
    , public ChatMessageItem {
public:
    ChatMessageTextItem(const MessageData *data);
    virtual void MarkAsDeleted();
};
