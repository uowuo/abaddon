#include "memberspane.hpp"

#include "abaddon.hpp"
#include "util.hpp"

GuildSettingsMembersPane::GuildSettingsMembersPane(Snowflake id)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , GuildID(id)
    , m_layout(Gtk::ORIENTATION_HORIZONTAL)
    , m_member_list(id)
    , m_member_info(id) {
    set_name("guild-members-pane");
    set_hexpand(true);
    set_vexpand(true);

    m_member_list.signal_member_select().connect(sigc::mem_fun(m_member_info, &GuildSettingsMembersPaneInfo::SetUser));

    m_note.set_label("Some members may not be shown if the client is not aware of them");
    m_note.set_single_line_mode(true);
    m_note.set_ellipsize(Pango::ELLIPSIZE_END);

    m_layout.set_homogeneous(true);
    m_layout.add(m_member_list);
    m_layout.add(m_member_info);
    add(m_note);
    add(m_layout);

    m_member_list.show();
    m_member_info.show();
    m_note.show();
    m_layout.show();
}

GuildSettingsMembersPaneMembers::GuildSettingsMembersPaneMembers(Snowflake id)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , GuildID(id) {
    m_list_scroll.get_style_context()->add_class("guild-members-pane-list");

    m_list_scroll.set_hexpand(true);
    m_list_scroll.set_vexpand(true);
    m_list_scroll.set_propagate_natural_height(true);

    auto &discord = Abaddon::Get().GetDiscordClient();
    auto members = discord.GetUsersInGuild(id);
    const auto guild = *discord.GetGuild(GuildID);
    for (const auto member_id : members) {
        auto member = discord.GetMember(member_id, GuildID);
        if (!member.has_value()) continue; // fixme this should not be necessary
        member->User = discord.GetUser(member_id);
        if (member->User->IsDeleted()) continue;
        auto *row = Gtk::manage(new GuildSettingsMembersListItem(guild, *member));
        row->show();
        m_list.add(*row);
    }

    m_list.set_selection_mode(Gtk::SELECTION_SINGLE);
    m_list.signal_row_selected().connect([this](Gtk::ListBoxRow *selected_) {
        if (auto *selected = dynamic_cast<GuildSettingsMembersListItem *>(selected_))
            m_signal_member_select.emit(selected->UserID);
    });

    m_search.set_placeholder_text("Filter");
    m_search.signal_changed().connect([this] {
        m_list.invalidate_filter();
    });

    m_list.set_filter_func([this](Gtk::ListBoxRow *row_) -> bool {
        const auto search_term = m_search.get_text();
        if (search_term.empty()) return true;
        if (auto *row = dynamic_cast<GuildSettingsMembersListItem *>(row_))
            return StringContainsCaseless(row->DisplayTerm, m_search.get_text());
        return true;
    });

    m_list_scroll.add(m_list);
    add(m_search);
    add(m_list_scroll);

    m_search.show();
    m_list.show();
    m_list_scroll.show();
}

GuildSettingsMembersPaneMembers::type_signal_member_select GuildSettingsMembersPaneMembers::signal_member_select() {
    return m_signal_member_select;
}

GuildSettingsMembersListItem::GuildSettingsMembersListItem(const GuildData &guild, const GuildMember &member)
    : UserID(member.User->ID)
    , GuildID(guild.ID)
    , m_avatar(32, 32) {
    m_avatar.SetAnimated(true);

    m_ev.signal_button_press_event().connect([this](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
            Abaddon::Get().ShowUserMenu(reinterpret_cast<GdkEvent *>(event), UserID, GuildID);
            return true;
        }
        return false;
    });

    auto &discord = Abaddon::Get().GetDiscordClient();

    if (member.User->HasAnimatedAvatar() && Abaddon::Get().GetSettings().ShowAnimations)
        m_avatar.SetURL(member.User->GetAvatarURL("gif", "32"));
    else
        m_avatar.SetURL(member.User->GetAvatarURL("png", "32"));

    DisplayTerm = member.User->GetUsername();

    const auto member_update_cb = [this](Snowflake guild_id, Snowflake user_id) {
        if (user_id == UserID)
            UpdateColor();
    };
    discord.signal_guild_member_update().connect(sigc::track_obj(member_update_cb, *this));
    UpdateColor();

    if (Abaddon::Get().GetSettings().ShowOwnerCrown && guild.OwnerID == member.User->ID) {
        try {
            const static auto crown_path = Abaddon::GetResPath("/crown.png");
            auto pixbuf = Gdk::Pixbuf::create_from_file(crown_path, 12, 12);
            m_crown = Gtk::manage(new Gtk::Image(pixbuf));
            m_crown->set_valign(Gtk::ALIGN_CENTER);
            m_crown->set_margin_start(10);
            m_crown->show();
        } catch (...) {}
    }

    m_avatar.set_margin_end(5);
    m_avatar.set_halign(Gtk::ALIGN_START);
    m_avatar.set_valign(Gtk::ALIGN_CENTER);
    m_name.set_halign(Gtk::ALIGN_START);
    m_name.set_valign(Gtk::ALIGN_CENTER);

    m_main.set_hexpand(true);

    m_main.add(m_avatar);
    m_main.add(m_name);
    if (m_crown != nullptr)
        m_main.add(*m_crown);

    m_ev.add(m_main);
    add(m_ev);

    m_avatar.show();
    m_name.show();
    m_main.show();
    m_ev.show();
}

void GuildSettingsMembersListItem::UpdateColor() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto user = *discord.GetUser(UserID);
    if (auto color_id = discord.GetMemberHoistedRole(GuildID, UserID, true); color_id.IsValid()) {
        auto role = *discord.GetRole(color_id);
        m_name.set_markup("<span color='#" + IntToCSSColor(role.Color) + "'>" + user.GetUsernameEscapedBold() + "</span>");
    } else
        m_name.set_markup(user.GetUsernameEscapedBold());
}

GuildSettingsMembersPaneInfo::GuildSettingsMembersPaneInfo(Snowflake guild_id)
    : GuildID(guild_id)
    , m_roles(guild_id)
    , m_box(Gtk::ORIENTATION_VERTICAL) {
    get_style_context()->add_class("guild-members-pane-info");

    const auto label = [](Gtk::Label &lbl) {
        lbl.set_single_line_mode(true);
        lbl.set_halign(Gtk::ALIGN_START);
        lbl.set_valign(Gtk::ALIGN_START);
        lbl.set_ellipsize(Pango::ELLIPSIZE_END);
        lbl.set_margin_bottom(5);
        lbl.show();
    };

    m_bot.set_text("User is a bot");

    label(m_bot);
    label(m_id);
    label(m_created);
    label(m_joined);
    label(m_nickname);
    label(m_boosting);

    m_box.set_halign(Gtk::ALIGN_FILL);
    m_box.set_valign(Gtk::ALIGN_START);
    m_box.set_hexpand(true);
    m_box.set_vexpand(true);
    m_box.add(m_bot);
    m_box.add(m_id);
    m_box.add(m_created);
    m_box.add(m_joined);
    m_box.add(m_nickname);
    m_box.add(m_boosting);
    m_box.add(m_roles);

    m_bot.hide();
    m_box.show();

    add(m_box);
}

void GuildSettingsMembersPaneInfo::SetUser(Snowflake user_id) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto guild = *discord.GetGuild(GuildID);
    auto member = *discord.GetMember(user_id, GuildID);
    member.User = discord.GetUser(user_id);

    m_bot.set_visible(member.User->IsABot());

    m_id.set_text("User ID: " + std::to_string(user_id));
    m_created.set_text("Account created: " + user_id.GetLocalTimestamp());
    if (!member.JoinedAt.empty())
        m_joined.set_text("Joined server: " + FormatISO8601(member.JoinedAt));
    else
        m_joined.set_text("Joined server: Unknown");
    m_nickname.set_text("Nickname: " + member.Nickname);
    m_nickname.set_visible(!member.Nickname.empty());
    if (member.PremiumSince.has_value()) {
        m_boosting.set_text("Boosting since " + FormatISO8601(*member.PremiumSince));
        m_boosting.show();
    } else
        m_boosting.hide();

    m_roles.show();
    m_roles.SetRoles(user_id, member.Roles, guild.OwnerID == discord.GetUserData().ID);
}

GuildSettingsMembersPaneRoles::GuildSettingsMembersPaneRoles(Snowflake guild_id)
    : GuildID(guild_id) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto self_id = discord.GetUserData().ID;
    const bool can_modify = discord.HasGuildPermission(self_id, guild_id, Permission::MANAGE_ROLES);
    const auto highest = discord.GetMemberHighestRole(GuildID, self_id);
    if (highest.has_value())
        m_hoisted_position = highest->Position;

    discord.signal_role_create().connect(sigc::mem_fun(*this, &GuildSettingsMembersPaneRoles::OnRoleCreate));
    discord.signal_role_update().connect(sigc::mem_fun(*this, &GuildSettingsMembersPaneRoles::OnRoleUpdate));
    discord.signal_role_delete().connect(sigc::mem_fun(*this, &GuildSettingsMembersPaneRoles::OnRoleDelete));

    const auto guild = *discord.GetGuild(guild_id);
    if (guild.Roles.has_value()) {
        for (const auto &role : *guild.Roles) {
            CreateRow(can_modify, role, guild.OwnerID == self_id);
        }
    }

    m_list.set_sort_func([](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b) -> int {
        auto *rowa = dynamic_cast<GuildSettingsMembersPaneRolesItem *>(a);
        auto *rowb = dynamic_cast<GuildSettingsMembersPaneRolesItem *>(b);
        return rowb->Position - rowa->Position;
    });

    set_propagate_natural_height(true);
    set_propagate_natural_width(true);
    set_hexpand(true);
    set_vexpand(true);
    set_halign(Gtk::ALIGN_FILL);
    set_valign(Gtk::ALIGN_START);

    m_list.show();

    add(m_list);
}

void GuildSettingsMembersPaneRoles::SetRoles(Snowflake user_id, const std::vector<Snowflake> &roles, bool is_owner) {
    UserID = user_id;

    for (auto it = m_update_connection.begin(); it != m_update_connection.end();) {
        it->disconnect();
        it = m_update_connection.erase(it);
    }

    m_set_role_ids = { roles.begin(), roles.end() };
    for (const auto &[role_id, row] : m_rows) {
        auto role = *Abaddon::Get().GetDiscordClient().GetRole(role_id);
        if (role.ID == GuildID) {
            row->SetChecked(true);
            row->SetToggleable(false);
        } else {
            row->SetToggleable(role.Position < m_hoisted_position || is_owner);
            row->SetChecked(m_set_role_ids.find(role_id) != m_set_role_ids.end());
        }
    }
}

void GuildSettingsMembersPaneRoles::CreateRow(bool has_manage_roles, const RoleData &role, bool is_owner) {
    auto *row = Gtk::manage(new GuildSettingsMembersPaneRolesItem(has_manage_roles, role));
    if (role.ID == GuildID) {
        row->SetChecked(true);
        row->SetToggleable(false);
    } else
        row->SetToggleable(role.Position < m_hoisted_position || is_owner);
    row->signal_role_click().connect(sigc::mem_fun(*this, &GuildSettingsMembersPaneRoles::OnRoleToggle));
    row->show();
    m_rows[role.ID] = row;
    m_list.add(*row);
}

void GuildSettingsMembersPaneRoles::OnRoleToggle(Snowflake role_id, bool new_set) {
    auto row = m_rows.at(role_id);
    row->SetToggleable(false);
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto cb = [this, new_set, role_id, row](bool success) {
        if (!success) { // undo
            if (new_set)
                m_set_role_ids.erase(role_id);
            else
                m_set_role_ids.insert(role_id);
        } else
            row->SetChecked(new_set);

        row->SetToggleable(true);
    };

    if (new_set)
        m_set_role_ids.insert(role_id);
    else
        m_set_role_ids.erase(role_id);

    // hack to prevent cb from being called if SetRoles is called before callback completion
    sigc::signal<void, bool> tmp;
    m_update_connection.emplace_back(tmp.connect(std::move(cb)));
    const auto tmp_cb = [tmp = std::move(tmp)](DiscordError code) { tmp.emit(code == DiscordError::NONE); };
    discord.SetMemberRoles(GuildID, UserID, m_set_role_ids.begin(), m_set_role_ids.end(), sigc::track_obj(tmp_cb, *this));
}

void GuildSettingsMembersPaneRoles::OnRoleCreate(Snowflake guild_id, Snowflake role_id) {
    if (guild_id != GuildID) return;
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto self_id = discord.GetUserData().ID;
    const bool can_modify = discord.HasGuildPermission(self_id, GuildID, Permission::MANAGE_ROLES);
    const auto role = *discord.GetRole(role_id);
    CreateRow(can_modify, role, discord.GetGuild(guild_id)->OwnerID == self_id);
}

void GuildSettingsMembersPaneRoles::OnRoleUpdate(Snowflake guild_id, Snowflake role_id) {
    if (guild_id != GuildID) return;
    auto role = *Abaddon::Get().GetDiscordClient().GetRole(role_id);
    m_rows.at(role_id)->UpdateRoleData(role);
    m_list.invalidate_sort();
}

void GuildSettingsMembersPaneRoles::OnRoleDelete(Snowflake guild_id, Snowflake role_id) {
    if (guild_id != GuildID) return;
    delete m_rows.at(role_id);
}

GuildSettingsMembersPaneRolesItem::GuildSettingsMembersPaneRolesItem(bool sensitive, const RoleData &role)
    : RoleID(role.ID) {
    UpdateRoleData(role);

    m_main.set_hexpand(true);
    m_main.set_vexpand(true);

    const auto cb = [this](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
            m_signal_role_click.emit(RoleID, !m_check.get_active());
            return true;
        }
        return false;
    };
    m_check.signal_button_press_event().connect(cb, false);

    m_desired_sensitivity = sensitive;
    ComputeSensitivity();

    m_check.set_margin_start(5);
    m_label.set_margin_start(5);

    m_main.add(m_check);
    m_main.add(m_label);
    add(m_main);
    m_check.show();
    m_label.show();
    m_main.show();
}

void GuildSettingsMembersPaneRolesItem::SetChecked(bool checked) {
    m_check.set_active(checked);
}

void GuildSettingsMembersPaneRolesItem::SetToggleable(bool toggleable) {
    m_desired_sensitivity = toggleable;
    ComputeSensitivity();
}

void GuildSettingsMembersPaneRolesItem::UpdateRoleData(const RoleData &role) {
    m_role = role;
    Position = role.Position;
    UpdateLabel();
}

void GuildSettingsMembersPaneRolesItem::UpdateLabel() {
    if (m_role.Color)
        m_label.set_markup("<span color='#" + IntToCSSColor(m_role.Color) + "'>" + Glib::Markup::escape_text(m_role.Name) + "</span>");
    else
        m_label.set_text(m_role.Name);
}

void GuildSettingsMembersPaneRolesItem::ComputeSensitivity() {
    if (m_role.IsManaged) {
        m_check.set_sensitive(false);
        return;
    }
    m_check.set_sensitive(m_desired_sensitivity);
}

GuildSettingsMembersPaneRolesItem::type_signal_role_click GuildSettingsMembersPaneRolesItem::signal_role_click() {
    return m_signal_role_click;
}
