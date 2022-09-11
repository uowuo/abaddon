#include "memberlist.hpp"
#include "abaddon.hpp"

MemberList::MemberList()
    : m_model(Gtk::TreeStore::create(m_columns)) {
    add(m_view);
    show_all_children();

    m_view.set_activate_on_single_click(true);
    m_view.set_hexpand(true);
    m_view.set_vexpand(true);

    m_view.set_show_expanders(false);
    m_view.set_enable_search(false);
    m_view.set_headers_visible(false);
    m_view.set_model(m_model);

    m_model->set_sort_column(m_columns.m_sort, Gtk::SORT_DESCENDING);

    auto *column = Gtk::make_managed<Gtk::TreeView::Column>("display");
    auto *renderer = Gtk::make_managed<CellRendererMemberList>();
    column->pack_start(*renderer);
    column->add_attribute(renderer->property_type(), m_columns.m_type);
    column->add_attribute(renderer->property_id(), m_columns.m_id);
    column->add_attribute(renderer->property_markup(), m_columns.m_markup);
    column->add_attribute(renderer->property_icon(), m_columns.m_icon);
    m_view.append_column(*column);

    m_view.expand_all();
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
    m_model->clear();
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (!discord.IsStarted()) return;

    const auto channel = discord.GetChannel(m_chan_id);
    if (!channel.has_value()) return;

    // dm
    if (channel->IsDM()) {
        // todo eliminate for dm
        auto everyone_row = *m_model->append();
        everyone_row[m_columns.m_type] = MemberListRenderType::Role;
        everyone_row[m_columns.m_id] = Snowflake::Invalid;
        everyone_row[m_columns.m_markup] = "<b>Users</b>";

        for (const auto &user : channel->GetDMRecipients()) {
            auto row = *m_model->append(everyone_row.children());
            row[m_columns.m_type] = MemberListRenderType::Member;
            row[m_columns.m_id] = user.ID;
            row[m_columns.m_markup] = Glib::Markup::escape_text(user.Username + "#" + user.Discriminator);
            row[m_columns.m_icon] = Abaddon::Get().GetImageManager().GetPlaceholder(16);
        }

        return;
    }

    std::set<Snowflake> ids;
    if (channel->IsThread()) {
        auto x = discord.GetUsersInThread(m_chan_id);
        ids = { x.begin(), x.end() };
    } else {
        ids = discord.GetUsersInGuild(m_guild_id);
    }

    std::map<int, RoleData> pos_to_role;
    std::map<int, std::vector<UserData>> pos_to_users;
    // std::unordered_map<Snowflake, int> user_to_color;
    std::vector<Snowflake> roleless_users;

    for (const auto &id : ids) {
        auto user = discord.GetUser(id);
        if (!user.has_value() || user->IsDeleted())
            continue;

        auto pos_role_id = discord.GetMemberHoistedRole(m_guild_id, id); // role for positioning
        // auto col_role_id = discord.GetMemberHoistedRole(m_guild_id, id, true); // role for color
        auto pos_role = discord.GetRole(pos_role_id);
        // auto col_role = discord.GetRole(col_role_id);

        if (!pos_role.has_value()) {
            roleless_users.push_back(id);
            continue;
        }

        pos_to_role[pos_role->Position] = *pos_role;
        pos_to_users[pos_role->Position].push_back(std::move(*user));
        // if (col_role.has_value())
        //    user_to_color[id] = col_role->Color;
    }

    const auto guild = discord.GetGuild(m_guild_id);
    if (!guild.has_value()) return;
    auto add_user = [this](const UserData &user, const Gtk::TreeNodeChildren &node) -> bool {
        auto row = *m_model->append(node);
        row[m_columns.m_type] = MemberListRenderType::Member;
        row[m_columns.m_id] = user.ID;
        row[m_columns.m_markup] = user.GetEscapedName() + "#" + user.Discriminator;
        row[m_columns.m_sort] = static_cast<int>(user.ID);
        row[m_columns.m_icon] = Abaddon::Get().GetImageManager().GetPlaceholder(16);
        // come on
        Gtk::TreeRowReference ref(m_model, m_model->get_path(row));
        Abaddon::Get().GetImageManager().LoadFromURL(user.GetAvatarURL(), [this, ref = std::move(ref)](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            if (ref.is_valid()) {
                auto row = *m_model->get_iter(ref.get_path());
                row[m_columns.m_icon] = pb->scale_simple(16, 16, Gdk::INTERP_BILINEAR);
            }
        });
        return true;
    };

    auto add_role = [this](const RoleData &role) -> Gtk::TreeRow {
        auto row = *m_model->append();
        row[m_columns.m_type] = MemberListRenderType::Role;
        row[m_columns.m_id] = role.ID;
        row[m_columns.m_markup] = "<b>" + role.GetEscapedName() + "</b>";
        row[m_columns.m_sort] = role.Position;
        return row;
    };

    for (auto &[pos, role] : pos_to_role) {
        auto role_children = add_role(role).children();
        if (auto it = pos_to_users.find(pos); it != pos_to_users.end()) {
            for (const auto &user : it->second) {
                if (!add_user(user, role_children)) break;
            }
        }
    }

    m_view.expand_all();
}

MemberList::ModelColumns::ModelColumns() {
    add(m_type);
    add(m_id);
    add(m_markup);
    add(m_icon);
    add(m_sort);
}
