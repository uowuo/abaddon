#pragma once
#include <gtkmm/widget.h>

class VolumeMeter : public Gtk::Widget {
public:
    VolumeMeter();

    void SetVolume(double fraction);
    void SetTick(double fraction);
    void SetShowTick(bool show);

protected:
    Gtk::SizeRequestMode get_request_mode_vfunc() const override;
    void get_preferred_width_vfunc(int &minimum_width, int &natural_width) const override;
    void get_preferred_width_for_height_vfunc(int height, int &minimum_width, int &natural_width) const override;
    void get_preferred_height_vfunc(int &minimum_height, int &natural_height) const override;
    void get_preferred_height_for_width_vfunc(int width, int &minimum_height, int &natural_height) const override;
    void on_size_allocate(Gtk::Allocation &allocation) override;
    void on_map() override;
    void on_unmap() override;
    void on_realize() override;
    void on_unrealize() override;
    bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

private:
    Glib::RefPtr<Gdk::Window> m_window;

    double m_fraction = 0.0;
    double m_tick = 0.0;
    bool m_show_tick = false;
};
