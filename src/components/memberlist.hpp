#pragma once
#include <unordered_map>

#include <gdkmm/pixbuf.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>

#include "cellrenderermemberlist.hpp"
#include "discord/user.hpp"
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
    bool OnButtonPressEvent(GdkEventButton *ev);

    void OnRoleSubmenuPopup();

    int SortFunc(const Gtk::TreeModel::iterator &a, const Gtk::TreeModel::iterator &b);

    void OnPresenceUpdate(const UserData &user, PresenceStatus status);

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns();

        Gtk::TreeModelColumn<MemberListRenderType> m_type;
        Gtk::TreeModelColumn<uint64_t> m_id;
        Gtk::TreeModelColumn<Glib::ustring> m_name;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> m_pixbuf;
        Gtk::TreeModelColumn<Gdk::RGBA> m_color;
        Gtk::TreeModelColumn<PresenceStatus> m_status;
        Gtk::TreeModelColumn<int> m_sort;

        Gtk::TreeModelColumn<bool> m_av_requested;
    };

    ModelColumns m_columns;
    Glib::RefPtr<Gtk::TreeStore> m_model;
    Gtk::TreeView m_view;

    Gtk::TreePath m_path_for_menu;

    Gtk::ScrolledWindow m_main;

    Snowflake m_active_channel;
    Snowflake m_active_guild;

    Gtk::Menu m_menu_role;
    Gtk::MenuItem m_menu_role_copy_id;

    std::unordered_map<Snowflake, Gtk::TreeIter> m_pending_avatars;
};
