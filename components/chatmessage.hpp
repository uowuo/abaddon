#pragma once
#include <gtkmm.h>
#include <string>
#include <sigc++/sigc++.h>
#include "../discord/discord.hpp"

enum class ChatDisplayType {
    Unknown,
    Text,
    Embed,
    Image,
};

// contains the username and timestamp, chat items get stuck into its box
class ChatMessageContainer : public Gtk::ListBoxRow {
public:
    Snowflake UserID;
    Snowflake ChannelID;

    ChatMessageContainer(const Message *data);
    void AddNewContent(Gtk::Widget *widget, bool prepend = false);
    void AddNewContentAtIndex(Gtk::Widget *widget, int index);
    void SetAvatarFromPixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf);
    void Update();
    int RemoveItem(Gtk::Widget *widget);

protected:
    Gtk::Box *m_main_box;
    Gtk::Box *m_content_box;
    Gtk::Box *m_meta_box;
    Gtk::Label *m_author;
    Gtk::Label *m_timestamp;
    Gtk::Image *m_avatar;
};

class ChatMessageItem {
public:
    ChatMessageItem();

    Snowflake ChannelID;
    Snowflake ID;
    ChatDisplayType MessageType = ChatDisplayType::Unknown;

    virtual void ShowMenu(const GdkEvent *event);
    void AddMenuItem(Gtk::MenuItem *item);
    virtual void Update() = 0;

    void SetContainer(ChatMessageContainer *container);
    ChatMessageContainer *GetContainer() const;

protected:
    ChatMessageContainer *m_container = nullptr;

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
    ChatMessageTextItem(const Message *data);

    void EditContent(std::string content);

    virtual void Update();

protected:
    void UpdateAttributes();

    std::string m_content;

    void on_menu_copy_content();
    Gtk::MenuItem *m_menu_copy_content;
    Gtk::MenuItem *m_menu_delete_message;
};

class ChatMessageEmbedItem
    : public Gtk::EventBox
    , public ChatMessageItem {
public:
    ChatMessageEmbedItem(const Message *data);

    virtual void Update();

protected:
    void DoLayout();
    void UpdateAttributes();

    EmbedData m_embed;
    Gtk::Box *m_main;
    Gtk::Label *m_attrib_label = nullptr;
};
