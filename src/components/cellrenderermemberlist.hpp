#pragma once
#include <gtkmm/cellrenderer.h>

enum class MemberListRenderType : uint8_t {
    Role,
    Member,
};

class CellRendererMemberList : public Gtk::CellRenderer {
public:
    CellRendererMemberList();
    ~CellRendererMemberList() override = default;

    Glib::PropertyProxy<MemberListRenderType> property_type();
    Glib::PropertyProxy<uint64_t> property_id();
    Glib::PropertyProxy<Glib::ustring> property_name();

protected:
    void get_preferred_width_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const override;
    void get_preferred_width_for_height_vfunc(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const override;
    void get_preferred_height_vfunc(Gtk::Widget &widget, int &minimum_height, int &natural_height) const override;
    void get_preferred_height_for_width_vfunc(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const override;
    void render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr,
                      Gtk::Widget &widget,
                      const Gdk::Rectangle &background_area,
                      const Gdk::Rectangle &cell_area,
                      Gtk::CellRendererState flags) override;

    void get_preferred_width_vfunc_role(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_role(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_role(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_role(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_role(const Cairo::RefPtr<Cairo::Context> &cr,
                           Gtk::Widget &widget,
                           const Gdk::Rectangle &background_area,
                           const Gdk::Rectangle &cell_area,
                           Gtk::CellRendererState flags);

    void get_preferred_width_vfunc_member(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_member(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_member(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_member(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_member(const Cairo::RefPtr<Cairo::Context> &cr,
                             Gtk::Widget &widget,
                             const Gdk::Rectangle &background_area,
                             const Gdk::Rectangle &cell_area,
                             Gtk::CellRendererState flags);

private:
    Gtk::CellRendererText m_renderer_text;
    Gtk::CellRendererPixbuf m_renderer_pixbuf;

    Glib::Property<MemberListRenderType> m_property_type;
    Glib::Property<uint64_t> m_property_id;
    Glib::Property<Glib::ustring> m_property_name;
};
