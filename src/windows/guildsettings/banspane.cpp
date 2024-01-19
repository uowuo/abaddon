#include "banspane.hpp"

#include <gtkmm/messagedialog.h>

#include "abaddon.hpp"

GuildSettingsBansPane::GuildSettingsBansPane(Snowflake id)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , GuildID(id)
    , m_model(Gtk::ListStore::create(m_columns))
    , m_menu_unban("Unban")
    , m_menu_copy_id("Copy ID") {
    signal_map().connect(sigc::mem_fun(*this, &GuildSettingsBansPane::OnMap));
    set_name("guild-bans-pane");
    set_hexpand(true);
    set_vexpand(true);

    auto &discord = Abaddon::Get().GetDiscordClient();

    discord.signal_guild_ban_add().connect(sigc::mem_fun(*this, &GuildSettingsBansPane::OnBanAdd));
    discord.signal_guild_ban_remove().connect(sigc::mem_fun(*this, &GuildSettingsBansPane::OnBanRemove));

    const auto self_id = discord.GetUserData().ID;
    const auto can_ban = discord.HasGuildPermission(self_id, GuildID, Permission::BAN_MEMBERS);

    if (!can_ban) {
        for (const auto &ban : discord.GetBansInGuild(id))
            OnGuildBanFetch(ban);

        m_no_perms_note = Gtk::manage(new Gtk::Label("You do not have permission to see bans. However, bans made while you are connected will appear here"));
        m_no_perms_note->set_single_line_mode(true);
        m_no_perms_note->set_ellipsize(Pango::ELLIPSIZE_END);
        m_no_perms_note->set_halign(Gtk::ALIGN_START);
        add(*m_no_perms_note);
    }

    m_menu_unban.signal_activate().connect(sigc::mem_fun(*this, &GuildSettingsBansPane::OnMenuUnban));
    m_menu_copy_id.signal_activate().connect(sigc::mem_fun(*this, &GuildSettingsBansPane::OnMenuCopyID));
    m_menu_unban.show();
    m_menu_copy_id.show();
    m_menu.append(m_menu_unban);
    m_menu.append(m_menu_copy_id);

    m_view.signal_button_press_event().connect(sigc::mem_fun(*this, &GuildSettingsBansPane::OnTreeButtonPress), false);
    m_view.show();

    m_scroll.set_propagate_natural_height(true);
    m_scroll.add(m_view);
    add(m_scroll);
    show_all_children();

    m_view.set_enable_search(false);
    m_view.set_model(m_model);
    m_view.append_column("User", m_columns.m_col_user);
    m_view.append_column("Reason", m_columns.m_col_reason);
}

void GuildSettingsBansPane::OnMap() {
    if (m_requested) return;
    m_requested = true;

    auto &discord = Abaddon::Get().GetDiscordClient();

    const auto self_id = discord.GetUserData().ID;
    const auto can_ban = discord.HasGuildPermission(self_id, GuildID, Permission::BAN_MEMBERS);

    if (can_ban)
        discord.FetchGuildBans(GuildID, sigc::mem_fun(*this, &GuildSettingsBansPane::OnGuildBansFetch));
}

void GuildSettingsBansPane::OnGuildBanFetch(const BanData &ban) {
    const auto user = Abaddon::Get().GetDiscordClient().GetUser(ban.User.ID);
    auto row = *m_model->append();
    row[m_columns.m_col_id] = ban.User.ID;
    if (user.has_value())
        row[m_columns.m_col_user] = user->GetUsername();
    else
        row[m_columns.m_col_user] = "<@" + std::to_string(ban.User.ID) + ">";

    row[m_columns.m_col_reason] = ban.Reason;
}

void GuildSettingsBansPane::OnGuildBansFetch(const std::vector<BanData> &bans) {
    for (const auto &ban : bans) {
        const auto user = Abaddon::Get().GetDiscordClient().GetUser(ban.User.ID);
        auto row = *m_model->append();
        row[m_columns.m_col_id] = user->ID;
        row[m_columns.m_col_user] = user->GetUsername();
        row[m_columns.m_col_reason] = ban.Reason;
    }
}

void GuildSettingsBansPane::OnMenuUnban() {
    auto selected_row = *m_view.get_selection()->get_selected();
    if (selected_row) {
        Snowflake id = selected_row[m_columns.m_col_id];
        auto cb = [](DiscordError code) {
            if (code != DiscordError::NONE) {
                Gtk::MessageDialog dlg("Failed to unban user", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
                dlg.set_position(Gtk::WIN_POS_CENTER);
                dlg.run();
            }
        };
        Abaddon::Get().GetDiscordClient().UnbanUser(GuildID, id, sigc::track_obj(cb, *this));
    }
}

void GuildSettingsBansPane::OnMenuCopyID() {
    auto selected_row = *m_view.get_selection()->get_selected();
    if (selected_row)
        Gtk::Clipboard::get()->set_text(std::to_string(static_cast<Snowflake>(selected_row[m_columns.m_col_id])));
}

bool GuildSettingsBansPane::OnTreeButtonPress(GdkEventButton *event) {
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
        auto &discord = Abaddon::Get().GetDiscordClient();
        const auto self_id = discord.GetUserData().ID;
        const auto can_ban = discord.HasGuildPermission(self_id, GuildID, Permission::BAN_MEMBERS);
        m_menu_unban.set_sensitive(can_ban);
        auto selection = m_view.get_selection();
        Gtk::TreeModel::Path path;
        if (m_view.get_path_at_pos(static_cast<int>(event->x), static_cast<int>(event->y), path)) {
            m_view.get_selection()->select(path);
            m_menu.popup_at_pointer(reinterpret_cast<GdkEvent *>(event));
        }

        return true;
    }

    return false;
}

void GuildSettingsBansPane::OnBanRemove(Snowflake guild_id, Snowflake user_id) {
    if (guild_id != GuildID) return;
    for (auto &child : m_model->children()) {
        if (static_cast<Snowflake>(child[m_columns.m_col_id]) == user_id) {
            m_model->erase(child);
            break;
        }
    }
}

void GuildSettingsBansPane::OnBanAdd(Snowflake guild_id, Snowflake user_id) {
    if (guild_id != GuildID) return;
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (discord.HasGuildPermission(discord.GetUserData().ID, guild_id, Permission::BAN_MEMBERS)) {
        discord.FetchGuildBan(guild_id, user_id, sigc::mem_fun(*this, &GuildSettingsBansPane::OnGuildBanFetch));
    } else {
        auto user = *discord.GetUser(user_id);
        auto row = *m_model->append();
        row[m_columns.m_col_id] = user_id;
        row[m_columns.m_col_user] = user.GetUsername();
        row[m_columns.m_col_reason] = "";
    }
}

GuildSettingsBansPane::ModelColumns::ModelColumns() {
    add(m_col_id);
    add(m_col_user);
    add(m_col_reason);
}
