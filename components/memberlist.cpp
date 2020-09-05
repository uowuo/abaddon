#include "memberlist.hpp"
#include "../abaddon.hpp"

MemberList::MemberList() {
    m_update_member_list_dispatcher.connect(sigc::mem_fun(*this, &MemberList::UpdateMemberListInternal));

    m_main = Gtk::manage(new Gtk::ScrolledWindow);
    m_listbox = Gtk::manage(new Gtk::ListBox);

    m_main->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    m_main->add(*m_listbox);
    m_main->show_all();
}

Gtk::Widget *MemberList::GetRoot() const {
    return m_main;
}

void MemberList::SetActiveChannel(Snowflake id) {
    std::scoped_lock<std::mutex> guard(m_mutex);
    m_chan_id = id;
    m_guild_id = m_abaddon->GetDiscordClient().GetChannel(id)->GuildID;
}

void MemberList::UpdateMemberList() {
    std::scoped_lock<std::mutex> guard(m_mutex);
    m_update_member_list_dispatcher.emit();
}

void MemberList::UpdateMemberListInternal() {
    auto children = m_listbox->get_children();
    auto it = children.begin();
    while (it != children.end()) {
        delete *it;
        it++;
    }

    if (!m_chan_id.IsValid()) return;

    auto &discord = m_abaddon->GetDiscordClient();
    auto *chan = discord.GetChannel(m_chan_id);
    std::unordered_set<Snowflake> ids;
    if (chan->Type == ChannelType::DM || chan->Type == ChannelType::GROUP_DM) {
        for (const auto &user : chan->Recipients)
            ids.insert(user.ID);
    } else {
        ids = discord.GetUsersInGuild(m_guild_id);
    }

    for (const auto &id : ids) {
        auto *user = discord.GetUser(id);
        auto *row = Gtk::manage(new Gtk::ListBoxRow);
        auto *label = Gtk::manage(new Gtk::Label);
        label->set_single_line_mode(true);
        label->set_ellipsize(Pango::ELLIPSIZE_END);
        if (user == nullptr)
            label->set_text("[unknown user]");
        else
            label->set_text(user->Username + "#" + user->Discriminator);
        label->set_halign(Gtk::ALIGN_START);
        row->add(*label);
        row->show_all();
        m_listbox->add(*row);
    }
}

void MemberList::SetAbaddon(Abaddon *ptr) {
    m_abaddon = ptr;
}
