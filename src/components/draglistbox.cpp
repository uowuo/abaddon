#include "draglistbox.hpp"
#include <glibmm/main.h>
#include <gtkmm/adjustment.h>

DragListBox::DragListBox() {
    drag_dest_set(m_entries, Gtk::DEST_DEFAULT_MOTION | Gtk::DEST_DEFAULT_DROP, Gdk::ACTION_MOVE);
}

void DragListBox::row_drag_begin(Gtk::Widget *widget, const Glib::RefPtr<Gdk::DragContext> &context) {
    m_drag_row = dynamic_cast<Gtk::ListBoxRow *>(widget->get_ancestor(GTK_TYPE_LIST_BOX_ROW));

    auto alloc = m_drag_row->get_allocation();
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, alloc.get_width(), alloc.get_height());
    auto cr = Cairo::Context::create(surface);

    m_drag_row->get_style_context()->add_class("drag-icon");
    gtk_widget_draw(reinterpret_cast<GtkWidget *>(m_drag_row->gobj()), cr->cobj());
    m_drag_row->get_style_context()->remove_class("drag-icon");

    int x, y;
    widget->translate_coordinates(*m_drag_row, 0, 0, x, y);
    surface->set_device_offset(-x, -y);
    context->set_icon(surface);
}

bool DragListBox::on_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, gint x, gint y, guint time) {
    if (y > m_hover_top || y < m_hover_bottom) {
        auto *row = get_row_at_y(y);
        if (row == nullptr) return true;
        const bool old_top = m_top;
        const auto alloc = row->get_allocation();

        const int hover_row_y = alloc.get_y();
        const int hover_row_height = alloc.get_height();
        if (row != m_drag_row) {
            if (y < hover_row_y + hover_row_height / 2) {
                m_hover_top = hover_row_y;
                m_hover_bottom = m_hover_top + hover_row_height / 2;
                row->get_style_context()->add_class("drag-hover-top");
                row->get_style_context()->remove_class("drag-hover-bottom");
                m_top = true;
            } else {
                m_hover_top = hover_row_y + hover_row_height / 2;
                m_hover_bottom = hover_row_y + hover_row_height;
                row->get_style_context()->add_class("drag-hover-bottom");
                row->get_style_context()->remove_class("drag-hover-top");
                m_top = false;
            }
        }

        if (m_hover_row != nullptr && m_hover_row != row) {
            if (old_top)
                m_hover_row->get_style_context()->remove_class("drag-hover-top");
            else
                m_hover_row->get_style_context()->remove_class("drag-hover-bottom");
        }

        m_hover_row = row;
    }

    check_scroll(y);
    if (m_should_scroll && !m_scrolling) {
        m_scrolling = true;
        Glib::signal_timeout().connect(sigc::mem_fun(*this, &DragListBox::scroll), SCROLL_DELAY);
    }

    return true;
}

void DragListBox::on_drag_leave(const Glib::RefPtr<Gdk::DragContext> &context, guint time) {
    m_should_scroll = false;
}

void DragListBox::check_scroll(gint y) {
    if (!get_focus_vadjustment())
        return;

    const double vadjustment_min = get_focus_vadjustment()->get_value();
    const double vadjustment_max = get_focus_vadjustment()->get_page_size() + vadjustment_min;
    const double show_min = std::max(0, y - SCROLL_DISTANCE);
    const double show_max = std::min(get_focus_vadjustment()->get_upper(), static_cast<double>(y) + SCROLL_DISTANCE);
    if (vadjustment_min > show_min) {
        m_should_scroll = true;
        m_scroll_up = true;
    } else if (vadjustment_max < show_max) {
        m_should_scroll = true;
        m_scroll_up = false;
    } else {
        m_should_scroll = false;
    }
}

bool DragListBox::scroll() {
    if (m_should_scroll) {
        if (m_scroll_up) {
            get_focus_vadjustment()->set_value(get_focus_vadjustment()->get_value() - SCROLL_STEP_SIZE);
        } else {
            get_focus_vadjustment()->set_value(get_focus_vadjustment()->get_value() + SCROLL_STEP_SIZE);
        }
    } else {
        m_scrolling = false;
    }
    return m_should_scroll;
}

void DragListBox::on_drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, const Gtk::SelectionData &selection_data, guint info, guint time) {
    int index;
    if (m_hover_row != nullptr) {
        if (m_top) {
            index = m_hover_row->get_index() - 1;
            m_hover_row->get_style_context()->remove_class("drag-hover-top");
        } else {
            index = m_hover_row->get_index();
            m_hover_row->get_style_context()->remove_class("drag-hover-bottom");
        }

        Gtk::Widget *handle = *reinterpret_cast<Gtk::Widget *const *>(selection_data.get_data());
        auto *row = dynamic_cast<Gtk::ListBoxRow *>(handle->get_ancestor(GTK_TYPE_LIST_BOX_ROW));

        if (row != nullptr && row != m_hover_row) {
            if (row->get_index() > index)
                index += 1;
            if (m_signal_on_drop.emit(row, index)) return;
            row->get_parent()->remove(*row);
            insert(*row, index);
        }
    }

    m_drag_row = nullptr;
}

void DragListBox::add_draggable(Gtk::ListBoxRow *widget) {
    widget->drag_source_set(m_entries, Gdk::BUTTON1_MASK, Gdk::ACTION_MOVE);
    widget->signal_drag_begin().connect(sigc::bind<0>(sigc::mem_fun(*this, &DragListBox::row_drag_begin), widget));
    widget->signal_drag_data_get().connect([widget](const Glib::RefPtr<Gdk::DragContext> &context, Gtk::SelectionData &selection_data, guint info, guint time) {
        selection_data.set("GTK_LIST_BOX_ROW", 32, reinterpret_cast<const guint8 *>(&widget), sizeof(&widget));
    });
    add(*widget);
}

DragListBox::type_signal_on_drop DragListBox::signal_on_drop() {
    return m_signal_on_drop;
}
