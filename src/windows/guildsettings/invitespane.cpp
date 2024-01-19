#include "invitespane.hpp"

#include <gtkmm/messagedialog.h>

#include "abaddon.hpp"
#include "util.hpp"

GuildSettingsInvitesPane::GuildSettingsInvitesPane(Snowflake id)
    : GuildID(id)
    , m_model(Gtk::ListStore::create(m_columns))
    , m_menu_delete("Delete") {
    signal_map().connect(sigc::mem_fun(*this, &GuildSettingsInvitesPane::OnMap));
    set_name("guild-invites-pane");
    set_hexpand(true);
    set_vexpand(true);

    m_view.signal_button_press_event().connect(sigc::mem_fun(*this, &GuildSettingsInvitesPane::OnTreeButtonPress), false);

    m_menu_delete.signal_activate().connect(sigc::mem_fun(*this, &GuildSettingsInvitesPane::OnMenuDelete));
    m_menu.append(m_menu_delete);
    m_menu.show_all();

    auto &discord = Abaddon::Get().GetDiscordClient();

    discord.signal_invite_create().connect(sigc::mem_fun(*this, &GuildSettingsInvitesPane::OnInviteCreate));
    discord.signal_invite_delete().connect(sigc::mem_fun(*this, &GuildSettingsInvitesPane::OnInviteDelete));

    m_view.show();
    add(m_view);

    m_view.set_enable_search(false);
    m_view.set_model(m_model);
    m_view.append_column("Code", m_columns.m_col_code);
    m_view.append_column("Expires", m_columns.m_col_expires);
    m_view.append_column("Created by", m_columns.m_col_inviter);
    m_view.append_column("Uses", m_columns.m_col_uses);
    m_view.append_column("Max uses", m_columns.m_col_max_uses);
    m_view.append_column("Grants temporary membership", m_columns.m_col_temporary);

    for (const auto column : m_view.get_columns())
        column->set_resizable(true);
}

void GuildSettingsInvitesPane::OnMap() {
    if (m_requested) return;
    m_requested = true;

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto self_id = discord.GetUserData().ID;

    if (discord.HasGuildPermission(self_id, GuildID, Permission::MANAGE_GUILD))
        discord.FetchGuildInvites(GuildID, sigc::mem_fun(*this, &GuildSettingsInvitesPane::OnInvitesFetch));
}

void GuildSettingsInvitesPane::AppendInvite(const InviteData &invite) {
    auto row = *m_model->append();
    row[m_columns.m_col_code] = invite.Code;
    if (invite.Inviter.has_value())
        row[m_columns.m_col_inviter] = invite.Inviter->GetUsername();

    if (invite.MaxAge.has_value()) {
        if (*invite.MaxAge == 0)
            row[m_columns.m_col_expires] = "Never";
        else
            row[m_columns.m_col_expires] = FormatISO8601(*invite.CreatedAt, *invite.MaxAge);
    }

    row[m_columns.m_col_uses] = *invite.Uses;
    if (*invite.MaxUses == 0)
        row[m_columns.m_col_max_uses] = "Unlimited";
    else
        row[m_columns.m_col_max_uses] = std::to_string(*invite.MaxUses);

    row[m_columns.m_col_temporary] = *invite.IsTemporary ? "Yes" : "No";
}

void GuildSettingsInvitesPane::OnInviteFetch(const std::optional<InviteData> &invite) {
    if (!invite.has_value()) return;
    AppendInvite(*invite);
}

void GuildSettingsInvitesPane::OnInvitesFetch(const std::vector<InviteData> &invites) {
    for (const auto &invite : invites)
        AppendInvite(invite);
}

void GuildSettingsInvitesPane::OnInviteCreate(const InviteData &invite) {
    if (invite.Guild->ID == GuildID)
        OnInviteFetch(std::make_optional(invite));
}

void GuildSettingsInvitesPane::OnInviteDelete(const InviteDeleteObject &data) {
    if (*data.GuildID == GuildID)
        for (auto &row : m_model->children())
            if (row[m_columns.m_col_code] == data.Code)
                m_model->erase(row);
}

void GuildSettingsInvitesPane::OnMenuDelete() {
    auto selected_row = *m_view.get_selection()->get_selected();
    if (selected_row) {
        auto code = static_cast<Glib::ustring>(selected_row[m_columns.m_col_code]);
        auto cb = [](DiscordError code) {
            if (code != DiscordError::NONE) {
                Gtk::MessageDialog dlg("Failed to delete invite", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
                dlg.set_position(Gtk::WIN_POS_CENTER);
                dlg.run();
            }
        };
        Abaddon::Get().GetDiscordClient().DeleteInvite(code, sigc::track_obj(cb, *this));
    }
}

bool GuildSettingsInvitesPane::OnTreeButtonPress(GdkEventButton *event) {
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
        auto &discord = Abaddon::Get().GetDiscordClient();
        const auto self_id = discord.GetUserData().ID;
        const auto can_manage = discord.HasGuildPermission(self_id, GuildID, Permission::MANAGE_GUILD);
        m_menu_delete.set_sensitive(can_manage);
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

GuildSettingsInvitesPane::ModelColumns::ModelColumns() {
    add(m_col_code);
    add(m_col_expires);
    add(m_col_inviter);
    add(m_col_temporary);
    add(m_col_uses);
    add(m_col_max_uses);
}
