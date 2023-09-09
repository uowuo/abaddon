#include "memberlist.hpp"

MemberList::MemberList()
    : m_model(Gtk::TreeStore::create(m_columns)) {
    m_main.get_style_context()->add_class("member-list");

    m_view.set_hexpand(true);
    m_view.set_vexpand(true);

    m_view.set_show_expanders(false);
    m_view.set_enable_search(false);
    m_view.set_headers_visible(false);
    m_view.get_selection()->set_mode(Gtk::SELECTION_NONE);
    m_view.set_model(m_model);

    m_main.add(m_view);
    m_main.show_all_children();

    auto *column = Gtk::make_managed<Gtk::TreeView::Column>("display");
    auto *renderer = Gtk::make_managed<CellRendererMemberList>();
    column->pack_start(*renderer);
    column->add_attribute(renderer->property_type(), m_columns.m_type);
    column->add_attribute(renderer->property_id(), m_columns.m_id);
    column->add_attribute(renderer->property_name(), m_columns.m_name);
    m_view.append_column(*column);
}

Gtk::Widget *MemberList::GetRoot() {
    return &m_main;
}

void MemberList::UpdateMemberList() {
    Clear();
    if (!m_active_channel.IsValid()) return;

    auto &discord = Abaddon::Get().GetDiscordClient();

    const auto channel = discord.GetChannel(m_active_channel);
    if (!channel.has_value()) {
        spdlog::get("ui")->warn("attempted to update member list with unfetchable channel");
        return;
    }

    if (channel->IsDM()) {
        for (const auto &user : channel->GetDMRecipients()) {
            auto row = *m_model->append();
            row[m_columns.m_type] = MemberListRenderType::Member;
            row[m_columns.m_id] = user.ID;
            row[m_columns.m_name] = user.GetDisplayNameEscaped();
        }
    }

    const auto guild = discord.GetGuild(m_active_guild);
    if (!guild.has_value()) return;

    std::set<Snowflake> ids;
    if (channel->IsThread()) {
        const auto x = discord.GetUsersInThread(m_active_channel);
        ids = { x.begin(), x.end() };
    } else {
        ids = discord.GetUsersInGuild(m_active_guild);
    }

    std::map<int, RoleData> pos_to_role;
    std::map<int, std::vector<UserData>> pos_to_users;
    std::unordered_map<Snowflake, int> user_to_color;
    std::vector<Snowflake> roleless_users;

    for (const auto user_id : ids) {
        auto user = discord.GetUser(user_id);
        if (!user.has_value() || user->IsDeleted()) continue;

        const auto pos_role_id = discord.GetMemberHoistedRole(m_active_guild, user_id);
        const auto col_role_id = discord.GetMemberHoistedRole(m_active_guild, user_id, true);
        const auto pos_role = discord.GetRole(pos_role_id);
        const auto col_role = discord.GetRole(col_role_id);

        if (!pos_role.has_value()) {
            roleless_users.push_back(user_id);
            continue;
        }

        pos_to_role[pos_role->Position] = *pos_role;
        pos_to_users[pos_role->Position].push_back(std::move(*user));
        if (col_role.has_value()) user_to_color[user_id] = col_role->Color;
    }

    const auto add_user = [this, &guild](const UserData &user, const Gtk::TreeRow &parent) {
        auto row = *m_model->append(parent->children());
        row[m_columns.m_type] = MemberListRenderType::Member;
        row[m_columns.m_id] = user.ID;
        row[m_columns.m_name] = user.GetDisplayNameEscaped();
        return row;
    };

    const auto add_role = [this](const RoleData &role) {
        auto row = *m_model->append();
        row[m_columns.m_type] = MemberListRenderType::Role;
        row[m_columns.m_id] = role.ID;
        row[m_columns.m_name] = role.GetEscapedName();
        return row;
    };

    for (auto it = pos_to_role.crbegin(); it != pos_to_role.crend(); it++) {
        const auto pos = it->first;
        const auto &role = it->second;

        auto role_row = add_role(role);

        if (pos_to_users.find(pos) == pos_to_users.end()) continue;

        auto &users = pos_to_users.at(pos);
        AlphabeticalSort(users.begin(), users.end(), [](const auto &e) { return e.Username; });

        for (const auto &user : users) add_user(user, role_row);
    }

    auto everyone_role = *m_model->append();
    everyone_role[m_columns.m_type] = MemberListRenderType::Role;
    everyone_role[m_columns.m_id] = m_active_guild; // yes thats how the role works
    everyone_role[m_columns.m_name] = "@everyone";

    for (const auto id : roleless_users) {
        const auto user = discord.GetUser(id);
        if (user.has_value()) add_user(*user, everyone_role);
    }

    m_view.expand_all();
}

void MemberList::Clear() {
    m_model->clear();
}

void MemberList::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
    m_active_guild = Snowflake::Invalid;
    if (m_active_channel.IsValid()) {
        const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(m_active_channel);
        if (channel.has_value() && channel->GuildID.has_value()) m_active_guild = *channel->GuildID;
    }
}

MemberList::ModelColumns::ModelColumns() {
    add(m_type);
    add(m_id);
    add(m_name);
}
