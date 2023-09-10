#pragma once
#include <gdkmm/pixbuf.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>

#include <unordered_map>

#include "cellrenderermemberlist.hpp"
#include "discord/snowflake.hpp"

class MemberList {
public:
    MemberList();
    Gtk::Widget *GetRoot();

    void UpdateMemberList();
    void Clear();
    void SetActiveChannel(Snowflake id);

private:
    void OnCellRender(uint64_t id);

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns();

        Gtk::TreeModelColumn<MemberListRenderType> m_type;
        Gtk::TreeModelColumn<uint64_t> m_id;
        Gtk::TreeModelColumn<Glib::ustring> m_name;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> m_pixbuf;
        Gtk::TreeModelColumn<Gdk::RGBA> m_color;

        Gtk::TreeModelColumn<bool> m_av_requested;
    };

    ModelColumns m_columns;
    Glib::RefPtr<Gtk::TreeStore> m_model;
    Gtk::TreeView m_view;

    Gtk::ScrolledWindow m_main;

    Snowflake m_active_channel;
    Snowflake m_active_guild;

    std::unordered_map<Snowflake, Gtk::TreeIter> m_pending_avatars;
};
