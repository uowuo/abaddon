#pragma once
#include <variant>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include "discord/snowflake.hpp"
#include "memberlistcellrenderer.hpp"

class MemberList : public Gtk::ScrolledWindow {
public:
    MemberList();

    void UpdateMemberList();
    void Clear();
    void SetActiveChannel(Snowflake id);

private:
    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns();

        Gtk::TreeModelColumn<MemberListRenderType> m_type;
        Gtk::TreeModelColumn<uint64_t> m_id;
        Gtk::TreeModelColumn<Glib::ustring> m_markup;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> m_icon;

        Gtk::TreeModelColumn<int> m_sort;
    };

    ModelColumns m_columns;
    Glib::RefPtr<Gtk::TreeStore> m_model;
    Gtk::TreeView m_view;

    Snowflake m_guild_id;
    Snowflake m_chan_id;
};
