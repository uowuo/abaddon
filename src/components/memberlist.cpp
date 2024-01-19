#include "memberlist.hpp"

#include "abaddon.hpp"
#include "util.hpp"

constexpr static int MemberListUserLimit = 200;

MemberList::MemberList()
    : m_model(Gtk::TreeStore::create(m_columns))
    , m_menu_role_copy_id("_Copy ID", true) {
    m_main.get_style_context()->add_class("member-list");

    m_view.set_hexpand(true);
    m_view.set_vexpand(true);

    m_view.set_show_expanders(false);
    m_view.set_enable_search(false);
    m_view.set_headers_visible(false);
    m_view.get_selection()->set_mode(Gtk::SELECTION_NONE);
    m_view.set_model(m_model);
    m_view.signal_button_press_event().connect(sigc::mem_fun(*this, &MemberList::OnButtonPressEvent), false);

    m_main.add(m_view);
    m_main.show_all_children();

    auto *column = Gtk::make_managed<Gtk::TreeView::Column>("display");
    auto *renderer = Gtk::make_managed<CellRendererMemberList>();
    column->pack_start(*renderer);
    column->add_attribute(renderer->property_type(), m_columns.m_type);
    column->add_attribute(renderer->property_id(), m_columns.m_id);
    column->add_attribute(renderer->property_name(), m_columns.m_name);
    column->add_attribute(renderer->property_pixbuf(), m_columns.m_pixbuf);
    column->add_attribute(renderer->property_color(), m_columns.m_color);
    column->add_attribute(renderer->property_status(), m_columns.m_status);
    m_view.append_column(*column);

    m_model->set_sort_column(m_columns.m_sort, Gtk::SORT_ASCENDING);
    m_model->set_default_sort_func([](const Gtk::TreeModel::iterator &, const Gtk::TreeModel::iterator &) -> int { return 0; });
    m_model->set_sort_func(m_columns.m_sort, sigc::mem_fun(*this, &MemberList::SortFunc));

    renderer->signal_render().connect(sigc::mem_fun(*this, &MemberList::OnCellRender));

    // Menu stuff

    m_menu_role.append(m_menu_role_copy_id);
    m_menu_role.show_all();

    m_menu_role_copy_id.signal_activate().connect([this]() {
        Gtk::Clipboard::get()->set_text(std::to_string((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });

    Abaddon::Get().GetDiscordClient().signal_presence_update().connect(sigc::mem_fun(*this, &MemberList::OnPresenceUpdate));
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
        return;
    }

    const static auto color_transparent = Gdk::RGBA("rgba(0,0,0,0)");

    if (channel->IsDM()) {
        for (const auto &user : channel->GetDMRecipients()) {
            auto row_iter = m_model->append();
            auto row = *row_iter;
            row[m_columns.m_type] = MemberListRenderType::Member;
            row[m_columns.m_id] = user.ID;
            row[m_columns.m_name] = user.GetDisplayNameEscaped();
            row[m_columns.m_color] = color_transparent;
            row[m_columns.m_av_requested] = false;
            row[m_columns.m_pixbuf] = Abaddon::Get().GetImageManager().GetPlaceholder(16);
            row[m_columns.m_status] = Abaddon::Get().GetDiscordClient().GetUserStatus(user.ID);
            m_pending_avatars[user.ID] = row_iter;
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

    std::unordered_map<Snowflake, std::vector<UserData>> role_to_users;
    std::unordered_map<Snowflake, int> user_to_color;
    std::vector<UserData> roleless_users;

    const auto users = discord.GetUsersBulk(ids.begin(), ids.end());

    std::unordered_map<Snowflake, RoleData> role_cache;
    if (guild->Roles.has_value()) {
        for (const auto &role : *guild->Roles) {
            role_cache[role.ID] = role;
        }
    }
    for (const auto &user : users) {
        if (user.IsDeleted()) continue;
        const auto member = discord.GetMember(user.ID, m_active_guild);
        if (!member.has_value()) continue;

        const auto pos_role = discord.GetMemberHoistedRoleCached(*member, role_cache);
        const auto col_role = discord.GetMemberHoistedRoleCached(*member, role_cache, true);

        if (!pos_role.has_value()) {
            roleless_users.push_back(user);
            continue;
        }

        role_to_users[pos_role->ID].push_back(user);
        if (col_role.has_value()) user_to_color[user.ID] = col_role->Color;
    }

    int count = 0;
    const auto add_user = [this, &count, &guild, &user_to_color](const UserData &user, const Gtk::TreeRow &parent) -> bool {
        if (count++ > MemberListUserLimit) return false;
        auto row_iter = m_model->append(parent->children());
        auto row = *row_iter;
        row[m_columns.m_type] = MemberListRenderType::Member;
        row[m_columns.m_id] = user.ID;
        row[m_columns.m_name] = user.GetDisplayNameEscaped();
        row[m_columns.m_pixbuf] = Abaddon::Get().GetImageManager().GetPlaceholder(16);
        row[m_columns.m_status] = Abaddon::Get().GetDiscordClient().GetUserStatus(user.ID);
        row[m_columns.m_av_requested] = false;
        if (const auto iter = user_to_color.find(user.ID); iter != user_to_color.end()) {
            row[m_columns.m_color] = IntToRGBA(iter->second);
        } else {
            row[m_columns.m_color] = color_transparent;
        }
        m_pending_avatars[user.ID] = row_iter;
        return true;
    };

    const auto add_role = [this](const RoleData &role) {
        auto row = *m_model->append();
        row[m_columns.m_type] = MemberListRenderType::Role;
        row[m_columns.m_id] = role.ID;
        row[m_columns.m_name] = "<b>" + role.GetEscapedName() + "</b>";
        row[m_columns.m_sort] = role.Position;
        return row;
    };

    // Kill sorting
    m_view.freeze_child_notify();
    m_model->set_sort_column(Gtk::TreeSortable::DEFAULT_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);

    for (const auto &[role_id, users] : role_to_users) {
        if (const auto iter = role_cache.find(role_id); iter != role_cache.end()) {
            auto role_row = add_role(iter->second);
            for (const auto &user : users) add_user(user, role_row);
        }
    }

    auto everyone_role = *m_model->append();
    everyone_role[m_columns.m_type] = MemberListRenderType::Role;
    everyone_role[m_columns.m_id] = m_active_guild; // yes thats how the role works
    everyone_role[m_columns.m_name] = "<b>@everyone</b>";
    everyone_role[m_columns.m_sort] = 0;

    for (const auto &user : roleless_users) {
        add_user(user, everyone_role);
    }

    // Restore sorting
    m_model->set_sort_column(m_columns.m_sort, Gtk::SORT_ASCENDING);
    m_view.expand_all();
    m_view.thaw_child_notify();
}

void MemberList::Clear() {
    m_model->clear();
    m_pending_avatars.clear();
}

void MemberList::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
    m_active_guild = Snowflake::Invalid;
    if (m_active_channel.IsValid()) {
        const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(m_active_channel);
        if (channel.has_value() && channel->GuildID.has_value()) m_active_guild = *channel->GuildID;
    }
}

void MemberList::OnCellRender(uint64_t id) {
    Snowflake real_id = id;
    if (const auto iter = m_pending_avatars.find(real_id); iter != m_pending_avatars.end()) {
        auto row = iter->second;
        m_pending_avatars.erase(iter);
        if (!row) return;
        if ((*row)[m_columns.m_av_requested]) return;
        (*row)[m_columns.m_av_requested] = true;
        const auto user = Abaddon::Get().GetDiscordClient().GetUser(real_id);
        if (!user.has_value()) return;
        const auto cb = [this, row](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            // for some reason row::operator bool() returns true when m_model->iter_is_valid returns false
            // idk why since other code already does essentially the same thing im doing here
            // iter_is_valid is "slow" according to gtk but the only other workaround i can think of would be worse
            if (row && m_model->iter_is_valid(row)) {
                (*row)[m_columns.m_pixbuf] = pb->scale_simple(16, 16, Gdk::INTERP_BILINEAR);
            }
        };
        Abaddon::Get().GetImageManager().LoadFromURL(user->GetAvatarURL("png", "16"), cb);
    }
}

bool MemberList::OnButtonPressEvent(GdkEventButton *ev) {
    if (ev->button == GDK_BUTTON_SECONDARY && ev->type == GDK_BUTTON_PRESS) {
        if (m_view.get_path_at_pos(static_cast<int>(ev->x), static_cast<int>(ev->y), m_path_for_menu)) {
            switch ((*m_model->get_iter(m_path_for_menu))[m_columns.m_type]) {
                case MemberListRenderType::Role:
                    OnRoleSubmenuPopup();
                    m_menu_role.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                    break;
                case MemberListRenderType::Member:
                    Abaddon::Get().ShowUserMenu(
                        reinterpret_cast<GdkEvent *>(ev),
                        static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]),
                        m_active_guild);
                    break;
            }
        }
        return true;
    }
    return false;
}

void MemberList::OnRoleSubmenuPopup() {
}

int MemberList::SortFunc(const Gtk::TreeModel::iterator &a, const Gtk::TreeModel::iterator &b) {
    if ((*a)[m_columns.m_type] == MemberListRenderType::Role) {
        return (*b)[m_columns.m_sort] - (*a)[m_columns.m_sort];
    } else if ((*a)[m_columns.m_type] == MemberListRenderType::Member) {
        return static_cast<Glib::ustring>((*a)[m_columns.m_name]).compare((*b)[m_columns.m_name]);
    }
    return 0;
}

void MemberList::OnPresenceUpdate(const UserData &user, PresenceStatus status) {
    for (auto &role : m_model->children()) {
        for (auto &member : role.children()) {
            if ((*member)[m_columns.m_id] == user.ID) {
                (*member)[m_columns.m_status] = status;
                return;
            }
        }
    }
}

MemberList::ModelColumns::ModelColumns() {
    add(m_type);
    add(m_id);
    add(m_name);
    add(m_pixbuf);
    add(m_color);
    add(m_status);
    add(m_sort);
    add(m_av_requested);
}
