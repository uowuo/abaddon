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
    void UpdateImage(std::string url, Glib::RefPtr<Gdk::Pixbuf> buf);

protected:
    bool EmitImageLoad(std::string url);

    void AddClickHandler(Gtk::Widget *widget, std::string);
    Gtk::TextView *CreateTextComponent(const Message *data); // Message.Content
    void UpdateTextComponent(Gtk::TextView *tv);
    Gtk::EventBox *CreateEmbedComponent(const Message *data); // Message.Embeds[0]
    Gtk::Image *CreateImageComponent(const AttachmentData &data);
    Gtk::Box *CreateAttachmentComponent(const AttachmentData &data); // non-image attachments
    void HandleImage(const AttachmentData &data, Gtk::Image *img, std::string url);

    // expects content run through Glib::Markup::escape_text
    std::string ParseMessageContent(std::string content);
    std::string ParseMentions(std::string content);

    void HandleChannelMentions(Gtk::TextView *tv);
    bool OnClickChannel(GdkEventButton *ev);

    void HandleLinks(Gtk::TextView *tv);
    bool OnLinkClick(GdkEventButton *ev);
    std::map<Glib::RefPtr<Gtk::TextTag>, std::string> m_link_tagmap;
    std::map<Glib::RefPtr<Gtk::TextTag>, Snowflake> m_channel_tagmap;

    std::unordered_map<std::string, std::pair<Gtk::Image *, AttachmentData>> m_img_loadmap;

    void AttachGuildMenuHandler(Gtk::Widget *widget);
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
    Gtk::Image *m_embed_img = nullptr; // yes this is hacky no i dont care (for now)
    std::string m_embed_imgurl;

    Gtk::TextView *m_text_component = nullptr;
    Gtk::EventBox *m_embed_component = nullptr;

public:
    typedef sigc::signal<void, std::string> type_signal_image_load;

    typedef sigc::signal<void> type_signal_action_delete;
    typedef sigc::signal<void> type_signal_action_edit;
    typedef sigc::signal<void, Snowflake> type_signal_channel_click;

    type_signal_action_delete signal_action_delete();
    type_signal_action_edit signal_action_edit();
    type_signal_channel_click signal_action_channel_click();

    type_signal_image_load signal_image_load();

private:
    type_signal_action_delete m_signal_action_delete;
    type_signal_action_edit m_signal_action_edit;
    type_signal_channel_click m_signal_action_channel_click;

    type_signal_image_load m_signal_image_load;
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
    Gtk::Label *m_extra = nullptr;
    Gtk::Image *m_avatar;
};
