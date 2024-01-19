#pragma once

#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/treeview.h>

#include "discord/emoji.hpp"

class GuildSettingsEmojisPane : public Gtk::Box {
public:
    GuildSettingsEmojisPane(Snowflake guild_id);

private:
    void OnMap();

    bool m_requested = false;

    void AddEmojiRow(const EmojiData &emoji);

    void OnFetchEmojis(std::vector<EmojiData> emojis);

    void OnEditName(Snowflake id, const std::string &name);
    void OnMenuCopyID();
    void OnMenuDelete();
    void OnMenuCopyEmojiURL();
    void OnMenuShowEmoji();
    bool OnTreeButtonPress(GdkEventButton *event);

    Snowflake GuildID;

    Gtk::Entry m_search;
    Gtk::ScrolledWindow m_view_scroll;
    Gtk::TreeView m_view;

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns();

        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> m_col_pixbuf;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::PixbufAnimation>> m_col_pixbuf_animation;
        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<Glib::ustring> m_col_creator;
        Gtk::TreeModelColumn<Glib::ustring> m_col_animated;
        Gtk::TreeModelColumn<Glib::ustring> m_col_available;
        Gtk::TreeModelColumn<Snowflake> m_col_id;
    };

    ModelColumns m_columns;
    Glib::RefPtr<Gtk::ListStore> m_model;
    Glib::RefPtr<Gtk::TreeModelFilter> m_filter;

    Gtk::Menu m_menu;
    Gtk::MenuItem m_menu_delete;
    Gtk::MenuItem m_menu_copy_id;
    Gtk::MenuItem m_menu_copy_emoji_url;
    Gtk::MenuItem m_menu_show_emoji;
};
