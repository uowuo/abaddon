#pragma once
#include <gtkmm.h>
#include "../discord/discord.hpp"

class ChatMessageItem : public Gtk::ListBoxRow {
public:
    Snowflake ID;
};

class ChatMessageTextItem : public ChatMessageItem {
public:
    ChatMessageTextItem(const MessageData* data);
};
