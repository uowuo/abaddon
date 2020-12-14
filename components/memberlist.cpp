#include "memberlist.hpp"
#include "../abaddon.hpp"
#include "../util.hpp"

MemberListUserRow::MemberListUserRow(Snowflake guild_id, const User *data) {
    ID = data->ID;
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_label = Gtk::manage(new Gtk::Label);
    m_avatar = Gtk::manage(new Gtk::Image(Abaddon::Get().GetImageManager().GetPlaceholder(16)));

    get_style_context()->add_class("members-row");
    get_style_context()->add_class("members-row-member");
    m_label->get_style_context()->add_class("members-row-label");
    m_avatar->get_style_context()->add_class("members-row-avatar");

    m_label->set_single_line_mode(true);
    m_label->set_ellipsize(Pango::ELLIPSIZE_END);
    if (data != nullptr) {
        static bool show_discriminator = Abaddon::Get().GetSettings().GetSettingBool("gui", "member_list_discriminator", true);
        std::string display = data->Username;
        if (show_discriminator)
            display += "#" + data->Discriminator;
        auto col_id = data->GetHoistedRole(guild_id, true);
        if (col_id.IsValid()) {
            auto color = Abaddon::Get().GetDiscordClient().GetRole(col_id)->Color;
            m_label->set_use_markup(true);
            m_label->set_markup("<span color='#" + IntToCSSColor(color) + "'>" + Glib::Markup::escape_text(display) + "</span>");

        } else {
            m_label->set_text(display);
        }
    } else {
        m_label->set_use_markup(true);
        m_label->set_markup("<i>[unknown user]</i>");
    }
    m_label->set_halign(Gtk::ALIGN_START);
    m_box->add(*m_avatar);
    m_box->add(*m_label);
    m_ev->add(*m_box);
    add(*m_ev);
    show_all();
}

void MemberListUserRow::SetAvatarFromPixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf) {
    m_avatar->property_pixbuf() = pixbuf;
}

MemberList::MemberList() {
    m_update_member_list_dispatcher.connect(sigc::mem_fun(*this, &MemberList::UpdateMemberListInternal));

    m_main = Gtk::manage(new Gtk::ScrolledWindow);
    m_listbox = Gtk::manage(new Gtk::ListBox);

    m_listbox->get_style_context()->add_class("members");

    m_listbox->set_selection_mode(Gtk::SELECTION_NONE);

    m_main->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    m_main->add(*m_listbox);
    m_main->show_all();
}

Gtk::Widget *MemberList::GetRoot() const {
    return m_main;
}

void MemberList::Clear() {
    SetActiveChannel(Snowflake::Invalid);
    UpdateMemberList();
}

void MemberList::SetActiveChannel(Snowflake id) {
    std::scoped_lock<std::mutex> guard(m_mutex);
    m_chan_id = id;
    m_guild_id = Snowflake::Invalid;
    if (m_chan_id.IsValid()) {
        const auto chan = Abaddon::Get().GetDiscordClient().GetChannel(id);
        if (chan.has_value()) m_guild_id = *chan->GuildID;
    }
}

void MemberList::UpdateMemberList() {
    std::scoped_lock<std::mutex> guard(m_mutex);
    m_update_member_list_dispatcher.emit();
}

void MemberList::UpdateMemberListInternal() {
    m_id_to_row.clear();

    auto children = m_listbox->get_children();
    auto it = children.begin();
    while (it != children.end()) {
        delete *it;
        it++;
    }

    if (!Abaddon::Get().GetDiscordClient().IsStarted()) return;
    if (!m_chan_id.IsValid()) return;

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto chan = discord.GetChannel(m_chan_id);
    if (!chan.has_value()) return;
    std::unordered_set<Snowflake> ids;
    if (chan->Type == ChannelType::DM || chan->Type == ChannelType::GROUP_DM) {
        for (const auto &user : chan->GetDMRecipients())
            ids.insert(user.ID);
        ids.insert(discord.GetUserData().ID);
    } else {
        ids = discord.GetUsersInGuild(m_guild_id);
    }

    // process all the shit first so its in proper order
    std::map<int, Role> pos_to_role;
    std::map<int, std::vector<User>> pos_to_users;
    std::unordered_map<Snowflake, int> user_to_color;
    std::vector<Snowflake> roleless_users;

    for (const auto &id : ids) {
        auto user = discord.GetUser(id);
        if (!user.has_value()) {
            roleless_users.push_back(id);
            continue;
        }

        auto pos_role_id = discord.GetMemberHoistedRole(m_guild_id, id);       // role for positioning
        auto col_role_id = discord.GetMemberHoistedRole(m_guild_id, id, true); // role for color
        auto pos_role = discord.GetRole(pos_role_id);
        auto col_role = discord.GetRole(col_role_id);

        if (!pos_role.has_value()) {
            roleless_users.push_back(id);
            continue;
        };

        pos_to_role[pos_role->Position] = *pos_role;
        pos_to_users[pos_role->Position].push_back(std::move(*user));
        if (col_role.has_value())
            user_to_color[id] = col_role->Color;
    }

    auto add_user = [this, &user_to_color](const User *data) {
        auto *row = Gtk::manage(new MemberListUserRow(m_guild_id, data));
        m_id_to_row[data->ID] = row;

        if (data->HasAvatar()) {
            auto buf = Abaddon::Get().GetImageManager().GetFromURLIfCached(data->GetAvatarURL("png", "16"));
            if (buf) {
                row->SetAvatarFromPixbuf(buf);
            } else {
                Snowflake id = data->ID;
                Abaddon::Get().GetImageManager().LoadFromURL(data->GetAvatarURL("png", "16"), [this, id](Glib::RefPtr<Gdk::Pixbuf> pbuf) {
                    Glib::signal_idle().connect([this, id, pbuf]() -> bool {
                        if (m_id_to_row.find(id) != m_id_to_row.end()) {
                            auto *foundrow = static_cast<MemberListUserRow *>(m_id_to_row.at(id));
                            if (foundrow != nullptr)
                                foundrow->SetAvatarFromPixbuf(pbuf);
                        }

                        return false;
                    });
                });
            }
        }

        AttachUserMenuHandler(row, data->ID);
        m_listbox->add(*row);
    };

    auto add_role = [this](std::string name) {
        auto *role_row = Gtk::manage(new Gtk::ListBoxRow);
        auto *role_lbl = Gtk::manage(new Gtk::Label);

        role_row->get_style_context()->add_class("members-row");
        role_row->get_style_context()->add_class("members-row-role");
        role_lbl->get_style_context()->add_class("members-row-label");

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
        const auto &role = it->second;

        add_role(role.Name);

        if (pos_to_users.find(pos) == pos_to_users.end()) continue;

        auto &users = pos_to_users.at(pos);
        AlphabeticalSort(users.begin(), users.end(), [](const auto &e) { return e.Username; });

        for (const auto data : users)
            add_user(&data);
    }

    if (chan->Type == ChannelType::DM || chan->Type == ChannelType::GROUP_DM)
        add_role("Users");
    else
        add_role("@everyone");
    for (const auto &id : roleless_users) {
        const auto user = discord.GetUser(id);
        if (user.has_value())
            add_user(&*user);
    }
}

void MemberList::AttachUserMenuHandler(Gtk::ListBoxRow *row, Snowflake id) {
    row->signal_button_press_event().connect([this, row, id](GdkEventButton *e) -> bool {
        if (e->type == GDK_BUTTON_PRESS && e->button == GDK_BUTTON_SECONDARY) {
            m_signal_action_show_user_menu.emit(reinterpret_cast<const GdkEvent *>(e), id, m_guild_id);
            return true;
        }

        return false;
    });
}

MemberList::type_signal_action_show_user_menu MemberList::signal_action_show_user_menu() {
    return m_signal_action_show_user_menu;
}
