#include "quickswitcherresultrow.hpp"

static constexpr int HeightRequest = 24; // probably change this part up

QuickSwitcherResultRow::QuickSwitcherResultRow(Snowflake id)
    : ID(id) {}

QuickSwitcherResultRowDM::QuickSwitcherResultRowDM(const ChannelData &channel)
    : QuickSwitcherResultRow(channel.ID)
    , m_img(channel.GetIconURL(), 24, 24, true) {
    set_size_request(-1, HeightRequest);

    m_label.set_text(channel.GetDisplayName());

    m_img.set_halign(Gtk::ALIGN_START);
    m_img.set_margin_right(5);
    m_label.set_halign(Gtk::ALIGN_START);

    m_box.pack_start(m_img, false, true);
    m_box.pack_start(m_label, true, true);
    add(m_box);
    show_all_children();
}

QuickSwitcherResultRowChannel::QuickSwitcherResultRowChannel(const ChannelData &channel)
    : QuickSwitcherResultRow(channel.ID) {
    set_size_request(-1, HeightRequest);

    m_label.set_text("#" + (channel.Name ? *channel.Name : std::string("???")));

    m_label.set_halign(Gtk::ALIGN_START);

    m_box.pack_start(m_label, true, true);
    add(m_box);
    show_all_children();
}

QuickSwitcherResultRowGuild::QuickSwitcherResultRowGuild(const GuildData &guild)
    : QuickSwitcherResultRow(guild.ID)
    , m_img(24, 24) {
    set_size_request(-1, HeightRequest);

    if (guild.HasIcon()) m_img.SetURL(guild.GetIconURL("png", "32"));
    m_label.set_text(guild.Name);

    m_img.set_halign(Gtk::ALIGN_START);
    m_img.set_margin_right(5);
    m_label.set_halign(Gtk::ALIGN_START);

    m_box.pack_start(m_img, false, true);
    m_box.pack_start(m_label, true, true);
    add(m_box);
    show_all_children();
}
