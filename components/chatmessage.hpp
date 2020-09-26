#pragma once
#include <gtkmm.h>
#include "../discord/discord.hpp"

class ChatMessageItemContainer : public Gtk::Box {
public:
    Snowflake ID;
    Snowflake ChannelID;

    ChatMessageItemContainer();
    static ChatMessageItemContainer *FromMessage(Snowflake id);

    // attributes = edited, deleted
    void UpdateAttributes();
    void UpdateContent();

protected:
    static Gtk::TextView *CreateTextComponent(const Message *data); // Message.Content
    static Gtk::EventBox *CreateEmbedComponent(const Message *data); // Message.Embeds[0]
    void AttachMenuHandler(Gtk::Widget *widget);
    void ShowMenu(GdkEvent *event);

    Gtk::Menu m_menu;
    Gtk::MenuItem *m_menu_copy_id;
    Gtk::MenuItem *m_menu_delete_message;
    Gtk::MenuItem *m_menu_edit_message;

    void on_menu_copy_id();
    void on_menu_delete_message();
    void on_menu_edit_message();

    Gtk::Box *m_main;
    Gtk::Label *m_attrib_label = nullptr;

    Gtk::TextView *m_text_component = nullptr;
    Gtk::EventBox *m_embed_component = nullptr;

public:
    typedef sigc::signal<void> type_signal_action_delete;
    typedef sigc::signal<void> type_signal_action_edit;

    type_signal_action_delete signal_action_delete();
    type_signal_action_edit signal_action_edit();

private:
    type_signal_action_delete m_signal_action_delete;
    type_signal_action_edit m_signal_action_edit;
};

class ChatMessageHeader : public Gtk::ListBoxRow {
public:
    Snowflake UserID;
    Snowflake ChannelID;

    ChatMessageHeader(const Message *data);
    void SetAvatarFromPixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf);
    void AddContent(Gtk::Widget *widget, bool prepend);
    void UpdateNameColor();

protected:
    Gtk::Box *m_main_box;
    Gtk::Box *m_content_box;
    Gtk::Box *m_meta_box;
    Gtk::Label *m_author;
    Gtk::Label *m_timestamp;
    Gtk::Image *m_avatar;
};
