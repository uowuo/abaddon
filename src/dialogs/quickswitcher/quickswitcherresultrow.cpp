#include "quickswitcherresultrow.hpp"

QuickSwitcherResultRow::QuickSwitcherResultRow(Snowflake id)
    : ID(id) {}

QuickSwitcherResultRowDM::QuickSwitcherResultRowDM(const ChannelData &channel)
    : QuickSwitcherResultRow(channel.ID)
    , m_img(channel.GetIconURL(), 24, 24, true) {
    m_label.set_text(channel.GetDisplayName());

    m_img.set_halign(Gtk::ALIGN_START);
    m_img.set_margin_right(5);
    m_label.set_halign(Gtk::ALIGN_START);

    m_box.pack_start(m_img, false, true);
    m_box.pack_start(m_label, true, true);
    add(m_box);
    show_all_children();
}
