#include "guildlistfolderitem.hpp"
#include "guildlistguilditem.hpp"

// doing my best to copy discord here

const int FolderGridButtonSize = 48;
const int FolderGridImageSize = 24;

class GuildListFolderButton : public Gtk::Grid {
public:
    GuildListFolderButton() {
        set_size_request(FolderGridButtonSize, FolderGridButtonSize);
    }

    void SetGuilds(const std::vector<Snowflake> &guild_ids) {
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++) {
                const size_t i = y * 2 + x;
                auto &widget = m_images[x][y];
                widget.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(FolderGridImageSize);
                attach(widget, x, y, 1, 1);

                if (i < guild_ids.size()) {
                    widget.show();

                    if (const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(guild_ids[i]); guild.has_value()) {
                        const auto cb = [&widget](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
                            widget.property_pixbuf() = pb->scale_simple(FolderGridImageSize, FolderGridImageSize, Gdk::INTERP_BILINEAR);
                        };
                        Abaddon::Get().GetImageManager().LoadFromURL(guild->GetIconURL("png", "32"), sigc::track_obj(cb, *this));
                    }
                }
            }
        }
    }

private:
    Gtk::Image m_images[2][2];
};

GuildListFolderItem::GuildListFolderItem(const UserSettingsGuildFoldersEntry &folder) {
    get_style_context()->add_class("classic-guild-folder");

    m_revealer.add(m_box);
    m_revealer.set_reveal_child(true);

    m_image.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(48);

    m_ev.signal_button_press_event().connect([this](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
            m_revealer.set_reveal_child(!m_revealer.get_reveal_child());
        }
        return false;
    });

    auto *btn = Gtk::make_managed<GuildListFolderButton>();
    btn->SetGuilds(folder.GuildIDs);
    m_ev.add(*btn);
    add(m_ev);
    add(m_revealer);

    btn->show();
    m_ev.show();
    m_revealer.show();
    m_box.show();
    m_image.show();
    show();
}

void GuildListFolderItem::AddGuildWidget(GuildListGuildItem *widget) {
    m_box.add(*widget);
}
