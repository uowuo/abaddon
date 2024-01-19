#pragma once
#include <unordered_map>
#include <gdkmm/pixbufanimation.h>
#include <glibmm/property.h>
#include <gtkmm/cellrenderer.h>

// handles both static and animated
class CellRendererPixbufAnimation : public Gtk::CellRenderer {
public:
    CellRendererPixbufAnimation();
    ~CellRendererPixbufAnimation() override = default;

    Glib::PropertyProxy<Glib::RefPtr<Gdk::Pixbuf>> property_pixbuf();
    Glib::PropertyProxy<Glib::RefPtr<Gdk::PixbufAnimation>> property_pixbuf_animation();

protected:
    void get_preferred_width_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const override;

    void get_preferred_width_for_height_vfunc(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const override;

    void get_preferred_height_vfunc(Gtk::Widget &widget, int &minimum_height, int &natural_height) const override;

    void get_preferred_height_for_width_vfunc(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const override;

    void render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr,
                      Gtk::Widget &widget,
                      const Gdk::Rectangle &background_area,
                      const Gdk::Rectangle &cell_area,
                      Gtk::CellRendererState flag) override;

private:
    Glib::Property<Glib::RefPtr<Gdk::Pixbuf>> m_property_pixbuf;
    Glib::Property<Glib::RefPtr<Gdk::PixbufAnimation>> m_property_pixbuf_animation;
    /* one cellrenderer is used for every animation and i dont know how to
       store data per-pixbuf (in this case the iter) so this little map thing will have to do
       i would try set_data on the pixbuf but i dont know if that will cause memory leaks
       this would mean if a row's pixbuf animation is changed more than once then it wont be released immediately
       but thats not a problem for me in this case

       unordered_map doesnt compile cuz theres no hash overload, i guess
    */
    std::map<Glib::RefPtr<Gdk::PixbufAnimation>, Glib::RefPtr<Gdk::PixbufAnimationIter>> m_pixbuf_animation_iters;
};
