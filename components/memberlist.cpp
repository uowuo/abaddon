#include "memberlist.hpp"
#include "../abaddon.hpp"
#include "../util.hpp"

MemberList::MemberList() {
    m_update_member_list_dispatcher.connect(sigc::mem_fun(*this, &MemberList::UpdateMemberListInternal));

    m_main = Gtk::manage(new Gtk::ScrolledWindow);
    m_listbox = Gtk::manage(new Gtk::ListBox);

    m_listbox->set_selection_mode(Gtk::SELECTION_NONE);

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
    printf("update member list\n");
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

    // process all the shit first so its in proper order
    std::map<int, const RoleData *> pos_to_role;
    std::map<int, std::vector<const UserData *>> pos_to_users;
    std::unordered_map<Snowflake, int> user_to_color;
    std::vector<Snowflake> roleless_users;

    for (const auto &id : ids) {
        auto *user = discord.GetUser(id);
        if (user == nullptr) {
            roleless_users.push_back(id);
            continue;
        }

        auto pos_role_id = discord.GetMemberHoistedRole(m_guild_id, id);       // role for positioning
        auto col_role_id = discord.GetMemberHoistedRole(m_guild_id, id, true); // role for color
        auto pos_role = discord.GetRole(pos_role_id);
        auto col_role = discord.GetRole(col_role_id);

        if (pos_role == nullptr) {
            roleless_users.push_back(id);
            continue;
        };

        pos_to_role[pos_role->Position] = pos_role;
        pos_to_users[pos_role->Position].push_back(user);
        if (col_role != nullptr) {
            if (ColorDistance(col_role->Color, 0xFFFFFF) < 15)
                user_to_color[id] = 0x000000;
            else
                user_to_color[id] = col_role->Color;
        }
    }

    auto add_user = [this, &user_to_color](const UserData *data) {
        auto *user_row = Gtk::manage(new Gtk::ListBoxRow);
        auto *user_lbl = Gtk::manage(new Gtk::Label);
        user_lbl->set_single_line_mode(true);
        user_lbl->set_ellipsize(Pango::ELLIPSIZE_END);
        if (data != nullptr) {
            std::string display = data->Username + "#" + data->Discriminator;
            if (user_to_color.find(data->ID) != user_to_color.end()) {
                auto color = user_to_color.at(data->ID);
                user_lbl->set_use_markup(true);
                user_lbl->set_markup("<span color='#" + IntToCSSColor(color) + "'>" + Glib::Markup::escape_text(display) + "</span>");

            } else {
                user_lbl->set_text(display);
            }
        } else {
            user_lbl->set_use_markup(true);
            user_lbl->set_markup("<i>[unknown user]</i>");
        }
        user_lbl->set_halign(Gtk::ALIGN_START);
        user_row->add(*user_lbl);
        user_row->show_all();
        m_listbox->add(*user_row);
    };

    auto add_role = [this](std::string name) {
        auto *role_row = Gtk::manage(new Gtk::ListBoxRow);
        auto *role_lbl = Gtk::manage(new Gtk::Label);
        role_lbl->set_single_line_mode(true);
        role_lbl->set_ellipsize(Pango::ELLIPSIZE_END);
        role_lbl->set_use_markup(true);
        role_lbl->set_markup("<b>" + Glib::Markup::escape_text(name) + "</b>");
        role_lbl->set_halign(Gtk::ALIGN_START);
        role_row->add(*role_lbl);
        role_row->show_all();
        m_listbox->add(*role_row);
    };

    for (auto it = pos_to_role.crbegin(); it != pos_to_role.crend(); it++) {
        auto pos = it->first;
        auto role = it->second;

        add_role(role->Name);

        if (pos_to_users.find(pos) == pos_to_users.end()) continue;

        auto &users = pos_to_users.at(pos);
        AlphabeticalSort(users.begin(), users.end(), [](auto e) { return e->Username; });

        for (const auto data : users)
            add_user(data);
    }

    add_role("@everyone");
    for (const auto &id : roleless_users) {
        add_user(discord.GetUser(id));
    }
}

void MemberList::SetAbaddon(Abaddon *ptr) {
    m_abaddon = ptr;
}
