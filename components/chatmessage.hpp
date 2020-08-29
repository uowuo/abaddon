#pragma once
#include <gtkmm.h>
#include "../discord/discord.hpp"

enum class ChatDisplayType {
    Unknown,
    Text,
};

class ChatMessageItem : public Gtk::ListBoxRow {
public:
    Snowflake ID;
    ChatDisplayType MessageType;

    virtual void MarkAsDeleted() = 0;
};

class ChatMessageTextItem : public ChatMessageItem {
public:
    ChatMessageTextItem(const MessageData *data);
    void AppendNewContent(std::string content);
    void PrependNewContent(std::string content);
    virtual void MarkAsDeleted();

protected:
    Gtk::Box *m_main_box;
    Gtk::Box *m_sub_box;
    Gtk::Box *m_meta_box;
    Gtk::Label *m_author;
    Gtk::Label *m_timestamp;
    Gtk::TextView *m_text;
};
