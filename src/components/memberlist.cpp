#include "memberlist.hpp"
#include "lazyimage.hpp"
#include "statusindicator.hpp"

constexpr static const int MaxMemberListRows = 200;

MemberListUserRow::MemberListUserRow(const std::optional<GuildData> &guild, const UserData &data) {
    ID = data.ID;
    m_ev = Gtk::manage(new Gtk::EventBox);
    m_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_label = Gtk::manage(new Gtk::Label);
    m_avatar = Gtk::manage(new LazyImage(16, 16));
    m_status_indicator = Gtk::manage(new StatusIndicator(ID));

    if (Abaddon::Get().GetSettings().ShowOwnerCrown && guild.has_value() && guild->OwnerID == data.ID) {
        try {
            const static auto crown_path = Abaddon::GetResPath("/crown.png");
            auto pixbuf = Gdk::Pixbuf::create_from_file(crown_path, 12, 12);
            m_crown = Gtk::manage(new Gtk::Image(pixbuf));
            m_crown->set_valign(Gtk::ALIGN_CENTER);
            m_crown->set_margin_end(8);
        } catch (...) {}
    }

    m_status_indicator->set_margin_start(3);

    if (guild.has_value())
        m_avatar->SetURL(data.GetAvatarURL(guild->ID, "png"));
    else
        m_avatar->SetURL(data.GetAvatarURL("png"));

    get_style_context()->add_class("members-row");
    get_style_context()->add_class("members-row-member");
    m_label->get_style_context()->add_class("members-row-label");
    m_avatar->get_style_context()->add_class("members-row-avatar");

    m_label->set_single_line_mode(true);
    m_label->set_ellipsize(Pango::ELLIPSIZE_END);

    // todo remove after migration complete
    std::string display;
    if (data.IsPomelo()) {
        display = data.GetDisplayName(guild.has_value() ? guild->ID : Snowflake::Invalid);
    } else {
        display = data.Username;
        if (Abaddon::Get().GetSettings().ShowMemberListDiscriminators) {
            display += "#" + data.Discriminator;
        }
    }

    if (guild.has_value()) {
        if (const auto col_id = data.GetHoistedRole(guild->ID, true); col_id.IsValid()) {
            auto color = Abaddon::Get().GetDiscordClient().GetRole(col_id)->Color;
            m_label->set_use_markup(true);
            m_label->set_markup("<span color='#" + IntToCSSColor(color) + "'>" + Glib::Markup::escape_text(display) + "</span>");
        } else {
            m_label->set_text(display);
        }
    } else {
        m_label->set_text(display);
    }

    m_label->set_halign(Gtk::ALIGN_START);
    m_box->add(*m_avatar);
    m_box->add(*m_status_indicator);
    m_box->add(*m_label);
    if (m_crown != nullptr)
        m_box->add(*m_crown);
    m_ev->add(*m_box);
    add(*m_ev);
    show_all();
}

MemberList::MemberList() {
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
    m_chan_id = id;
    m_guild_id = Snowflake::Invalid;
    if (m_chan_id.IsValid()) {
        const auto chan = Abaddon::Get().GetDiscordClient().GetChannel(id);
        if (chan.has_value() && chan->GuildID.has_value()) m_guild_id = *chan->GuildID;
    }
}

void MemberList::UpdateMemberList() {
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
    if (chan->Type == ChannelType::DM || chan->Type == ChannelType::GROUP_DM) {
        int num_rows = 0;
        for (const auto &user : chan->GetDMRecipients()) {
            if (num_rows++ > MaxMemberListRows) break;
            auto *row = Gtk::manage(new MemberListUserRow(std::nullopt, user));
            m_id_to_row[user.ID] = row;
            AttachUserMenuHandler(row, user.ID);
            m_listbox->add(*row);
        }

        return;
    }

    std::set<Snowflake> ids;
    if (chan->IsThread()) {
        const auto x = discord.GetUsersInThread(m_chan_id);
        ids = { x.begin(), x.end() };
    } else
        ids = discord.GetUsersInGuild(m_guild_id);

    // process all the shit first so its in proper order
    std::map<int, RoleData> pos_to_role;
    std::map<int, std::vector<UserData>> pos_to_users;
    std::unordered_map<Snowflake, int> user_to_color;
    std::vector<Snowflake> roleless_users;

    for (const auto &id : ids) {
        auto user = discord.GetUser(id);
        if (!user.has_value() || user->IsDeleted())
            continue;

        auto pos_role_id = discord.GetMemberHoistedRole(m_guild_id, id);       // role for positioning
        auto col_role_id = discord.GetMemberHoistedRole(m_guild_id, id, true); // role for color
        auto pos_role = discord.GetRole(pos_role_id);
        auto col_role = discord.GetRole(col_role_id);

        if (!pos_role.has_value()) {
            roleless_users.push_back(id);
            continue;
        }

        pos_to_role[pos_role->Position] = *pos_role;
        pos_to_users[pos_role->Position].push_back(std::move(*user));
        if (col_role.has_value())
            user_to_color[id] = col_role->Color;
    }

    int num_rows = 0;
    const auto guild = discord.GetGuild(m_guild_id);
    if (!guild.has_value()) return;
    auto add_user = [this, &num_rows, guild](const UserData &data) -> bool {
        if (num_rows++ > MaxMemberListRows) return false;
        auto *row = Gtk::manage(new MemberListUserRow(*guild, data));
        m_id_to_row[data.ID] = row;
        AttachUserMenuHandler(row, data.ID);
        m_listbox->add(*row);
        return true;
    };

    auto add_role = [this](const std::string &name) {
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

        for (const auto &data : users)
            if (!add_user(data)) return;
    }

    if (chan->Type == ChannelType::DM || chan->Type == ChannelType::GROUP_DM)
        add_role("Users");
    else
        add_role("@everyone");
    for (const auto &id : roleless_users) {
        const auto user = discord.GetUser(id);
        if (user.has_value())
            if (!add_user(*user)) return;
    }
}

void MemberList::AttachUserMenuHandler(Gtk::ListBoxRow *row, Snowflake id) {
    row->signal_button_press_event().connect([this, id](GdkEventButton *e) -> bool {
        if (e->type == GDK_BUTTON_PRESS && e->button == GDK_BUTTON_SECONDARY) {
            Abaddon::Get().ShowUserMenu(reinterpret_cast<const GdkEvent *>(e), id, m_guild_id);
            return true;
        }

        return false;
    });
}
