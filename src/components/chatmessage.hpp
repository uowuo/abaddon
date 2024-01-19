#pragma once
#include <gdkmm/pixbufanimation.h>
#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/listboxrow.h>
#include <gtkmm/textview.h>
#include "discord/discord.hpp"

class ChatMessageItemContainer : public Gtk::EventBox {
public:
    Snowflake ID;
    Snowflake ChannelID;

    std::string Nonce;

    ChatMessageItemContainer();
    static ChatMessageItemContainer *FromMessage(const Message &data);

    // attributes = edited, deleted
    void UpdateAttributes();
    void UpdateContent();
    void UpdateReactions();
    void SetFailed();

protected:
    static void AddClickHandler(Gtk::Widget *widget, const std::string &);
    Gtk::TextView *CreateTextComponent(const Message &data); // Message.Content
    void UpdateTextComponent(Gtk::TextView *tv);
    Gtk::Widget *CreateEmbedsComponent(const std::vector<EmbedData> &embeds);
    static Gtk::Widget *CreateEmbedComponent(const EmbedData &data); // Message.Embeds[0]
    Gtk::Widget *CreateImageComponent(const std::string &proxy_url, const std::string &url, int inw, int inh);
    Gtk::Widget *CreateAttachmentComponent(const AttachmentData &data); // non-image attachments
    Gtk::Widget *CreateStickersComponent(const std::vector<StickerItem> &data);
    Gtk::Widget *CreateReactionsComponent(const Message &data);
    Gtk::Widget *CreateReplyComponent(const Message &data);

    static bool IsEmbedImageOnly(const EmbedData &data);

    void HandleChannelMentions(const Glib::RefPtr<Gtk::TextBuffer> &buf);
    void HandleChannelMentions(Gtk::TextView *tv);
    bool OnClickChannel(GdkEventButton *ev);
    bool OnTextViewButtonPress(GdkEventButton *ev);

    // reused for images and links
    Gtk::Menu m_link_menu;
    Gtk::MenuItem *m_link_menu_copy;

    void on_link_menu_copy();
    Glib::ustring m_selected_link;

    void HandleLinks(Gtk::TextView &tv);
    bool OnLinkClick(GdkEventButton *ev);
    std::map<Glib::RefPtr<Gtk::TextTag>, std::string> m_link_tagmap;
    std::map<Glib::RefPtr<Gtk::TextTag>, Snowflake> m_channel_tagmap;

    Gtk::EventBox *_ev;
    Gtk::Box m_main;
    Gtk::Label *m_attrib_label = nullptr;

    Gtk::TextView *m_text_component = nullptr;
    Gtk::Widget *m_embed_component = nullptr;
    Gtk::Widget *m_reactions_component = nullptr;

public:
    typedef sigc::signal<void, Snowflake> type_signal_channel_click;
    typedef sigc::signal<void, Glib::ustring> type_signal_action_reaction_add;
    typedef sigc::signal<void, Glib::ustring> type_signal_action_reaction_remove;

    type_signal_channel_click signal_action_channel_click();
    type_signal_action_reaction_add signal_action_reaction_add();
    type_signal_action_reaction_remove signal_action_reaction_remove();

private:
    type_signal_channel_click m_signal_action_channel_click;
    type_signal_action_reaction_add m_signal_action_reaction_add;
    type_signal_action_reaction_remove m_signal_action_reaction_remove;
};

class ChatMessageHeader : public Gtk::ListBoxRow {
public:
    Snowflake UserID;
    Snowflake ChannelID;
    Snowflake NewestID = 0;

    ChatMessageHeader(const Message &data);
    void AddContent(Gtk::Widget *widget, bool prepend);
    void UpdateName();
    std::vector<Gtk::Widget *> GetChildContent();

protected:
    void AttachUserMenuHandler(Gtk::Widget &widget);

    bool on_author_button_press(GdkEventButton *ev);

    std::vector<Gtk::Widget *> m_content_widgets;

    Gtk::Box m_main_box;
    Gtk::Box m_content_box;
    Gtk::EventBox m_content_box_ev;
    Gtk::Box m_meta_box;
    Gtk::EventBox m_meta_ev;
    Gtk::Label m_author;
    Gtk::Label m_timestamp;
    Gtk::Label *m_extra = nullptr;
    Gtk::Image m_avatar;
    Gtk::EventBox m_avatar_ev;

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
