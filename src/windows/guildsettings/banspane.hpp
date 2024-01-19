#pragma once

#include <gtkmm/box.h>
#include <gtkmm/liststore.h>
#include <gtkmm/menu.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>

#include "discord/snowflake.hpp"
#include "discord/ban.hpp"

class GuildSettingsBansPane : public Gtk::Box {
public:
    GuildSettingsBansPane(Snowflake id);

private:
    void OnMap();

    bool m_requested = false;

    void OnGuildBanFetch(const BanData &ban);
    void OnGuildBansFetch(const std::vector<BanData> &bans);
    void OnMenuUnban();
    void OnMenuCopyID();
    bool OnTreeButtonPress(GdkEventButton *event);
    void OnBanRemove(Snowflake guild_id, Snowflake user_id);
    void OnBanAdd(Snowflake guild_id, Snowflake user_id);

    Gtk::Label *m_no_perms_note = nullptr;

    Gtk::ScrolledWindow m_scroll;
    Gtk::TreeView m_view;

    Snowflake GuildID;

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns();

        Gtk::TreeModelColumn<Glib::ustring> m_col_user;
        Gtk::TreeModelColumn<Glib::ustring> m_col_reason;
        Gtk::TreeModelColumn<Snowflake> m_col_id;
    };

    ModelColumns m_columns;
    Glib::RefPtr<Gtk::ListStore> m_model;

    Gtk::Menu m_menu;
    Gtk::MenuItem m_menu_unban;
    Gtk::MenuItem m_menu_copy_id;
};
