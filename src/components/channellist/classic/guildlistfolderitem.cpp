#include "guildlistfolderitem.hpp"

GuildListFolderItem::GuildListFolderItem() {
    m_revealer.add(m_box);
    m_revealer.set_reveal_child(true);

    m_ev.signal_button_press_event().connect([this](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
            m_revealer.set_reveal_child(!m_revealer.get_reveal_child());
        }
        return false;
    });

    m_ev.add(m_image);
    add(m_ev);
    add(m_revealer);
    show_all_children();
}
