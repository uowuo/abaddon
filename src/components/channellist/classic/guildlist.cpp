#include "guildlist.hpp"

#include "abaddon.hpp"
#include "util.hpp"
#include "guildlistfolderitem.hpp"

class GuildListDMsButton : public Gtk::EventBox {
public:
    GuildListDMsButton() {
        set_size_request(48, 48);

        m_img.property_icon_name() = "user-available-symbolic"; // meh
        m_img.property_icon_size() = Gtk::ICON_SIZE_DND;
        add(m_img);
        show_all_children();
    }

private:
    Gtk::Image m_img;
};

GuildList::GuildList()
    : m_menu_guild_copy_id("_Copy ID", true)
    , m_menu_guild_settings("View _Settings", true)
    , m_menu_guild_leave("_Leave", true)
    , m_menu_guild_mark_as_read("Mark as _Read", true) {
    get_style_context()->add_class("classic-guild-list");
    set_selection_mode(Gtk::SELECTION_NONE);
    show_all_children();

    m_menu_guild_copy_id.signal_activate().connect([this] {
        Gtk::Clipboard::get()->set_text(std::to_string(m_menu_guild_target));
    });
    m_menu_guild_settings.signal_activate().connect([this] {
        m_signal_action_guild_settings.emit(m_menu_guild_target);
    });
    m_menu_guild_leave.signal_activate().connect([this] {
        m_signal_action_guild_leave.emit(m_menu_guild_target);
    });
    m_menu_guild_mark_as_read.signal_activate().connect([this] {
        Abaddon::Get().GetDiscordClient().MarkGuildAsRead(m_menu_guild_target, [](...) {});
    });
    m_menu_guild_toggle_mute.signal_activate().connect([this] {
        const auto id = m_menu_guild_target;
        auto &discord = Abaddon::Get().GetDiscordClient();
        if (discord.IsGuildMuted(id))
            discord.UnmuteGuild(id, NOOP_CALLBACK);
        else
            discord.MuteGuild(id, NOOP_CALLBACK);
    });
    m_menu_guild.append(m_menu_guild_mark_as_read);
    m_menu_guild.append(m_menu_guild_settings);
    m_menu_guild.append(m_menu_guild_leave);
    m_menu_guild.append(m_menu_guild_toggle_mute);
    m_menu_guild.append(m_menu_guild_copy_id);
    m_menu_guild.show_all();
}

void GuildList::UpdateListing() {
    auto &discord = Abaddon::Get().GetDiscordClient();

    Clear();

    auto *dms = Gtk::make_managed<GuildListDMsButton>();
    dms->show();
    dms->signal_button_press_event().connect([this](GdkEventButton *ev) -> bool {
        if (ev->type == GDK_BUTTON_PRESS && ev->button == GDK_BUTTON_PRIMARY) {
            m_signal_dms_selected.emit();
        }
        return false;
    });
    add(*dms);

    // does this function still even work ??lol
    const auto folders = discord.GetUserSettings().GuildFolders;
    const auto guild_ids = discord.GetUserSortedGuilds();

    // same logic from ChannelListTree

    std::set<Snowflake> foldered_guilds;
    for (const auto &group : folders) {
        foldered_guilds.insert(group.GuildIDs.begin(), group.GuildIDs.end());
    }

    for (auto iter = guild_ids.crbegin(); iter != guild_ids.crend(); iter++) {
        if (foldered_guilds.find(*iter) == foldered_guilds.end()) {
            AddGuild(*iter);
        }
    }

    for (const auto &group : folders) {
        AddFolder(group);
    }
}

void GuildList::AddGuild(Snowflake id) {
    if (auto item = CreateGuildWidget(id)) {
        item->show();
        add(*item);
    }
}

GuildListGuildItem *GuildList::CreateGuildWidget(Snowflake id) {
    const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(id);
    if (!guild.has_value()) return nullptr;

    auto *item = Gtk::make_managed<GuildListGuildItem>(*guild);
    item->signal_button_press_event().connect([this, id](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
            m_signal_guild_selected.emit(id);
        } else if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
            m_menu_guild_target = id;
            OnGuildSubmenuPopup();
            m_menu_guild.popup_at_pointer(reinterpret_cast<GdkEvent *>(event));
        }
        return true;
    });

    return item;
}

void GuildList::AddFolder(const UserSettingsGuildFoldersEntry &folder) {
    // groups with no ID arent actually folders
    if (!folder.ID.has_value()) {
        if (!folder.GuildIDs.empty()) {
            AddGuild(folder.GuildIDs[0]);
        }
        return;
    }

    auto *folder_widget = Gtk::make_managed<GuildListFolderItem>(folder);
    for (const auto guild_id : folder.GuildIDs) {
        if (auto *guild_widget = CreateGuildWidget(guild_id)) {
            guild_widget->show();
            folder_widget->AddGuildWidget(guild_widget);
        }
    }

    folder_widget->show();
    add(*folder_widget);
}

void GuildList::Clear() {
    const auto children = get_children();
    for (auto *child : children) {
        delete child;
    }
}

void GuildList::OnGuildSubmenuPopup() {
    const auto id = m_menu_guild_target;
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (discord.IsGuildMuted(id)) {
        m_menu_guild_toggle_mute.set_label("Unmute");
    } else {
        m_menu_guild_toggle_mute.set_label("Mute");
    }

    const auto guild = discord.GetGuild(id);
    const auto self_id = discord.GetUserData().ID;
    m_menu_guild_leave.set_sensitive(!(guild.has_value() && guild->OwnerID == self_id));
}

GuildList::type_signal_guild_selected GuildList::signal_guild_selected() {
    return m_signal_guild_selected;
}

GuildList::type_signal_dms_selected GuildList::signal_dms_selected() {
    return m_signal_dms_selected;
}

GuildList::type_signal_action_guild_leave GuildList::signal_action_guild_leave() {
    return m_signal_action_guild_leave;
}

GuildList::type_signal_action_guild_settings GuildList::signal_action_guild_settings() {
    return m_signal_action_guild_settings;
}
