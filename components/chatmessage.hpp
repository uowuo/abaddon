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
    void UpdateReactions();

protected:
    bool EmitImageLoad(std::string url);

    void AddClickHandler(Gtk::Widget *widget, std::string);
    Gtk::TextView *CreateTextComponent(const Message *data); // Message.Content
    void UpdateTextComponent(Gtk::TextView *tv);
    Gtk::Widget *CreateEmbedComponent(const Message *data); // Message.Embeds[0]
    Gtk::Widget *CreateImageComponent(const AttachmentData &data);
    Gtk::Widget *CreateAttachmentComponent(const AttachmentData &data); // non-image attachments
    Gtk::Widget *CreateStickerComponent(const Sticker &data);
    Gtk::Widget *CreateReactionsComponent(const Message *data);
    void ReactionUpdateImage(Gtk::Image *img, const Glib::RefPtr<Gdk::Pixbuf> &pb);
    void HandleImage(const AttachmentData &data, Gtk::Image *img, std::string url);

    void OnEmbedImageLoad(const Glib::RefPtr<Gdk::Pixbuf> &pixbuf);

    static Glib::ustring GetText(const Glib::RefPtr<Gtk::TextBuffer> &buf);

    void HandleUserMentions(Gtk::TextView *tv);
    void HandleStockEmojis(Gtk::TextView *tv);
    void HandleCustomEmojis(Gtk::TextView *tv);
    void HandleEmojis(Gtk::TextView *tv);

    void HandleChannelMentions(Gtk::TextView *tv);
    bool OnClickChannel(GdkEventButton *ev);

    // reused for images and links
    Gtk::Menu m_link_menu;
    Gtk::MenuItem *m_link_menu_copy;

    void on_link_menu_copy();
    Glib::ustring m_selected_link;

    void HandleLinks(Gtk::TextView *tv);
    bool OnLinkClick(GdkEventButton *ev);
    std::map<Glib::RefPtr<Gtk::TextTag>, std::string> m_link_tagmap;
    std::map<Glib::RefPtr<Gtk::TextTag>, Snowflake> m_channel_tagmap;

    std::unordered_map<std::string, std::pair<Gtk::Image *, AttachmentData>> m_img_loadmap;

    void AttachEventHandlers(Gtk::Widget *widget);
    void ShowMenu(GdkEvent *event);

    Gtk::Menu m_menu;
    Gtk::MenuItem *m_menu_copy_id;
    Gtk::MenuItem *m_menu_copy_content;
    Gtk::MenuItem *m_menu_delete_message;
    Gtk::MenuItem *m_menu_edit_message;

    void on_menu_copy_id();
    void on_menu_delete_message();
    void on_menu_edit_message();
    void on_menu_copy_content();

    Gtk::EventBox *m_ev;
    Gtk::Box *m_main;
    Gtk::Label *m_attrib_label = nullptr;
    Gtk::Image *m_embed_img = nullptr; // yes this is hacky no i dont care (for now)
    std::string m_embed_imgurl;

    Gtk::TextView *m_text_component = nullptr;
    Gtk::Widget *m_embed_component = nullptr;
    Gtk::Widget *m_reactions_component = nullptr;

public:
    typedef sigc::signal<void, std::string> type_signal_image_load;

    typedef sigc::signal<void> type_signal_action_delete;
    typedef sigc::signal<void> type_signal_action_edit;
    typedef sigc::signal<void, Snowflake> type_signal_channel_click;
    typedef sigc::signal<void, Glib::ustring> type_signal_action_reaction_add;
    typedef sigc::signal<void, Glib::ustring> type_signal_action_reaction_remove;
    typedef sigc::signal<void> type_signal_enter;
    typedef sigc::signal<void> type_signal_leave;

    type_signal_action_delete signal_action_delete();
    type_signal_action_edit signal_action_edit();
    type_signal_channel_click signal_action_channel_click();
    type_signal_action_reaction_add signal_action_reaction_add();
    type_signal_action_reaction_remove signal_action_reaction_remove();
    type_signal_enter signal_enter();
    type_signal_leave signal_leave();

    type_signal_image_load signal_image_load();

private:
    type_signal_action_delete m_signal_action_delete;
    type_signal_action_edit m_signal_action_edit;
    type_signal_channel_click m_signal_action_channel_click;
    type_signal_action_reaction_add m_signal_action_reaction_add;
    type_signal_action_reaction_remove m_signal_action_reaction_remove;
    type_signal_enter m_signal_enter;
    type_signal_leave m_signal_leave;

    type_signal_image_load m_signal_image_load;
};

class ChatMessageHeader : public Gtk::ListBoxRow {
public:
    Snowflake UserID;
    Snowflake ChannelID;

    ChatMessageHeader(const Message *data);
    void AddContent(Gtk::Widget *widget, bool prepend);
    void UpdateNameColor();

protected:
    void OnAvatarLoad(const Glib::RefPtr<Gdk::Pixbuf> &pixbuf);
    void OnAnimatedAvatarLoad(const Glib::RefPtr<Gdk::PixbufAnimation> &pixbuf);

    void AttachUserMenuHandler(Gtk::Widget &widget);

    bool on_author_button_press(GdkEventButton *ev);

    Gtk::Box *m_main_box;
    Gtk::Box *m_content_box;
    Gtk::Box *m_meta_box;
    Gtk::EventBox *m_meta_ev;
    Gtk::Label *m_author;
    Gtk::Label *m_timestamp;
    Gtk::Label *m_extra = nullptr;
    Gtk::Image *m_avatar;
    Gtk::EventBox *m_avatar_ev;

    Glib::RefPtr<Gdk::Pixbuf> m_static_avatar;
    Glib::RefPtr<Gdk::PixbufAnimation> m_anim_avatar;

    typedef sigc::signal<void> type_signal_action_insert_mention;
    typedef sigc::signal<void, const GdkEvent *> type_signal_action_open_user_menu;

    type_signal_action_insert_mention m_signal_action_insert_mention;
    type_signal_action_open_user_menu m_signal_action_open_user_menu;

public:
    type_signal_action_insert_mention signal_action_insert_mention();
    type_signal_action_open_user_menu signal_action_open_user_menu();
};
