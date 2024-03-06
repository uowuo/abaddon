#include "guildlistfolderitem.hpp"

#include "abaddon.hpp"
#include "guildlistguilditem.hpp"
#include "util.hpp"

// doing my best to copy discord here

const int FolderGridButtonSize = 48;
const int FolderGridImageSize = 24;

GuildListFolderButton::GuildListFolderButton() {
    set_size_request(FolderGridButtonSize, FolderGridButtonSize);
}

void GuildListFolderButton::SetGuilds(const std::vector<Snowflake> &guild_ids) {
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            const size_t i = y * 2 + x;
            auto &widget = m_images[x][y];
            widget.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(FolderGridImageSize);
            attach(widget, x, y, 1, 1);

            if (i < guild_ids.size()) {
                widget.show();

                if (const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(guild_ids[i]); guild.has_value() && guild->HasIcon()) {
                    const auto cb = [&widget](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
                        widget.property_pixbuf() = pb->scale_simple(FolderGridImageSize, FolderGridImageSize, Gdk::INTERP_BILINEAR);
                    };
                    Abaddon::Get().GetImageManager().LoadFromURL(guild->GetIconURL("png", "32"), sigc::track_obj(cb, *this));
                }
            }
        }
    }
}

GuildListFolderItem::GuildListFolderItem(const UserSettingsGuildFoldersEntry &folder) {
    m_guild_ids = folder.GuildIDs;

    get_style_context()->add_class("classic-guild-list-folder");

    if (folder.Name.has_value()) {
        set_tooltip_text(*folder.Name);
    }

    m_revealer.add(m_box);
    m_revealer.set_reveal_child(false);

    m_image.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(48);

    m_ev.signal_button_press_event().connect([this](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
            m_revealer.set_reveal_child(!m_revealer.get_reveal_child());
            if (!Abaddon::Get().GetSettings().FolderIconOnly) {
                if (m_revealer.get_reveal_child()) {
                    m_stack.set_visible_child("icon", Gtk::STACK_TRANSITION_TYPE_SLIDE_DOWN);
                } else {
                    m_stack.set_visible_child("grid", Gtk::STACK_TRANSITION_TYPE_SLIDE_UP);
                }
            }
        }

        return false;
    });

    m_grid.SetGuilds(folder.GuildIDs);
    m_grid.show();

    m_icon.property_icon_name() = "folder-symbolic";
    m_icon.property_icon_size() = Gtk::ICON_SIZE_DND;
    if (folder.Color.has_value()) {
        m_icon.override_color(IntToRGBA(*folder.Color));
    }
    m_icon.show();

    m_stack.add(m_grid, "grid");
    m_stack.add(m_icon, "icon");
    m_stack.set_visible_child(Abaddon::Get().GetSettings().FolderIconOnly ? "icon" : "grid");
    m_stack.show();

    m_ev.add(m_stack);
    add(m_ev);
    add(m_revealer);

    m_ev.show();
    m_revealer.show();
    m_box.show();
    m_image.show();
    show();

    Abaddon::Get().GetDiscordClient().signal_message_create().connect(sigc::mem_fun(*this, &GuildListFolderItem::OnMessageCreate));
    Abaddon::Get().GetDiscordClient().signal_message_ack().connect(sigc::mem_fun(*this, &GuildListFolderItem::OnMessageAck));

    CheckUnreadStatus();
}

void GuildListFolderItem::AddGuildWidget(GuildListGuildItem *widget) {
    m_box.add(*widget);
}

void GuildListFolderItem::OnMessageCreate(const Message &msg) {
    if (msg.GuildID.has_value() && std::find(m_guild_ids.begin(), m_guild_ids.end(), *msg.GuildID) != m_guild_ids.end()) CheckUnreadStatus();
}

void GuildListFolderItem::OnMessageAck(const MessageAckData &data) {
    CheckUnreadStatus();
}

void GuildListFolderItem::CheckUnreadStatus() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (!Abaddon::Get().GetSettings().Unreads) return;

    bool has_any_unreads = false;

    for (auto guild_id : m_guild_ids) {
        int mentions;
        if (!discord.IsGuildMuted(guild_id) && discord.GetUnreadStateForGuild(guild_id, mentions)) {
            has_any_unreads = true;
            break;
        }
    }

    if (has_any_unreads) {
        get_style_context()->add_class("has-unread");
    } else {
        get_style_context()->remove_class("has-unread");
    }
}
