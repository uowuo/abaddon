#pragma once
#include "discord/snowflake.hpp"
#include "discord/activity.hpp"

class StatusIndicator : public Gtk::Widget {
public:
    StatusIndicator(Snowflake user_id);
    ~StatusIndicator() override = default;

protected:
    Gtk::SizeRequestMode get_request_mode_vfunc() const override;
    void get_preferred_width_vfunc(int &minimum_width, int &natural_width) const override;
    void get_preferred_height_for_width_vfunc(int width, int &minimum_height, int &natural_height) const override;
    void get_preferred_height_vfunc(int &minimum_height, int &natural_height) const override;
    void get_preferred_width_for_height_vfunc(int height, int &minimum_width, int &natural_width) const override;
    void on_size_allocate(Gtk::Allocation &allocation) override;
    void on_map() override;
    void on_unmap() override;
    void on_realize() override;
    void on_unrealize() override;
    bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

    Glib::RefPtr<Gdk::Window> m_window;

    void CheckStatus();

    Snowflake m_id;
    PresenceStatus m_status;
};
