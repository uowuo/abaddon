#pragma once
#include <gtkmm/listbox.h>

class DragListBox : public Gtk::ListBox {
public:
    DragListBox();

    void row_drag_begin(Gtk::Widget *widget, const Glib::RefPtr<Gdk::DragContext> &context);

    bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, gint x, gint y, guint time) override;

    void on_drag_leave(const Glib::RefPtr<Gdk::DragContext> &context, guint time) override;

    void check_scroll(gint y);

    bool scroll();

    void on_drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, const Gtk::SelectionData &selection_data, guint info, guint time) override;

    void add_draggable(Gtk::ListBoxRow *widget);

private:
    Gtk::ListBoxRow *m_hover_row = nullptr;
    Gtk::ListBoxRow *m_drag_row = nullptr;
    bool m_top = false;
    int m_hover_top = 0;
    int m_hover_bottom = 0;
    bool m_should_scroll = false;
    bool m_scrolling = false;
    bool m_scroll_up = false;

    constexpr static int SCROLL_STEP_SIZE = 8;
    constexpr static int SCROLL_DISTANCE = 30;
    constexpr static int SCROLL_DELAY = 50;

    const std::vector<Gtk::TargetEntry> m_entries = {
        Gtk::TargetEntry("GTK_LIST_BOX_ROW", Gtk::TARGET_SAME_APP, 0),
    };

    using type_signal_on_drop = sigc::signal<bool, Gtk::ListBoxRow *, int /* new index */>;
    type_signal_on_drop m_signal_on_drop;

public:
    type_signal_on_drop signal_on_drop(); // return true to prevent drop
};
