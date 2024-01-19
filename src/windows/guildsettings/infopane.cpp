#include "infopane.hpp"

#include <filesystem>

#include <gdkmm/pixbufloader.h>
#include <gtkmm/filechoosernative.h>
#include <gtkmm/messagedialog.h>

#include "abaddon.hpp"
#include "util.hpp"

GuildSettingsInfoPane::GuildSettingsInfoPane(Snowflake id)
    : m_guild_name_label("Guild name")
    , GuildID(id) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto guild = *discord.GetGuild(id);
    const auto self_id = discord.GetUserData().ID;
    const auto can_modify = discord.HasGuildPermission(self_id, id, Permission::MANAGE_GUILD);

    set_name("guild-info-pane");

    m_guild_name.set_sensitive(can_modify);
    m_guild_name.set_text(guild.Name);
    m_guild_name.signal_focus_out_event().connect([this](GdkEventFocus *e) -> bool {
        UpdateGuildName();
        return false;
    });
    m_guild_name.signal_key_press_event().connect([this](GdkEventKey *e) -> bool {
        if (e->keyval == GDK_KEY_Return)
            UpdateGuildName();
        return false;
        // clang-format off
    }, false);
    // clang-format on
    m_guild_name.set_tooltip_text("Press enter or lose focus to submit");
    m_guild_name.show();
    m_guild_name_label.show();

    auto guild_update_cb = [this](Snowflake id) {
        if (id != GuildID) return;
        const auto guild = *Abaddon::Get().GetDiscordClient().GetGuild(id);
        FetchGuildIcon(guild);
    };
    discord.signal_guild_update().connect(sigc::track_obj(guild_update_cb, *this));
    FetchGuildIcon(guild);

    AddPointerCursor(m_guild_icon_ev);

    m_guild_icon.set_margin_bottom(10);
    if (can_modify) {
        m_guild_icon_ev.set_tooltip_text("Click to choose a file, right click to paste");

        m_guild_icon_ev.signal_button_press_event().connect([this](GdkEventButton *event) -> bool {
            if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
                UpdateGuildIconPicker();
            }

            return false;
        });
    } else if (guild.HasIcon()) {
        std::string guild_icon_url;
        if (guild.HasAnimatedIcon())
            guild_icon_url = guild.GetIconURL("gif", "512");
        else
            guild_icon_url = guild.GetIconURL("png", "512");
        m_guild_icon_ev.signal_button_release_event().connect([guild_icon_url](GdkEventButton *event) -> bool {
            if (event->type == GDK_BUTTON_RELEASE && event->button == GDK_BUTTON_PRIMARY)
                LaunchBrowser(guild_icon_url);

            return false;
        });
    }

    m_guild_icon.show();
    m_guild_icon_ev.show();

    m_guild_icon_ev.add(m_guild_icon);
    attach(m_guild_icon_ev, 0, 0, 1, 1);
    attach(m_guild_name_label, 0, 1, 1, 1);
    attach_next_to(m_guild_name, m_guild_name_label, Gtk::POS_RIGHT, 1, 1);
}

void GuildSettingsInfoPane::FetchGuildIcon(const GuildData &guild) {
    m_guild_icon.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(32);
    if (guild.HasIcon()) {
        if (Abaddon::Get().GetSettings().ShowAnimations && guild.HasAnimatedIcon()) {
            auto cb = [this](const Glib::RefPtr<Gdk::PixbufAnimation> &pixbuf) {
                m_guild_icon.property_pixbuf_animation() = pixbuf;
            };
            Abaddon::Get().GetImageManager().LoadAnimationFromURL(guild.GetIconURL("gif", "64"), 64, 64, sigc::track_obj(cb, *this));
        }

        auto cb = [this](const Glib::RefPtr<Gdk::Pixbuf> &pixbuf) {
            m_guild_icon.property_pixbuf() = pixbuf->scale_simple(64, 64, Gdk::INTERP_BILINEAR);
        };
        Abaddon::Get().GetImageManager().LoadFromURL(guild.GetIconURL("png", "64"), sigc::track_obj(cb, *this));
    }
}

void GuildSettingsInfoPane::UpdateGuildName() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (discord.GetGuild(GuildID)->Name == m_guild_name.get_text()) return;

    auto cb = [this](DiscordError code) {
        if (code != DiscordError::NONE) {
            m_guild_name.set_text(Abaddon::Get().GetDiscordClient().GetGuild(GuildID)->Name);
            Gtk::MessageDialog dlg("Failed to set guild name", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
            dlg.set_position(Gtk::WIN_POS_CENTER);
            dlg.run();
        }
    };
    discord.SetGuildName(GuildID, m_guild_name.get_text(), sigc::track_obj(cb, *this));
}

void GuildSettingsInfoPane::UpdateGuildIconFromData(const std::vector<uint8_t> &data, const std::string &mime) {
    auto encoded = "data:" + mime + ";base64," + Glib::Base64::encode(std::string(data.begin(), data.end()));
    auto &discord = Abaddon::Get().GetDiscordClient();

    auto cb = [](DiscordError code) {
        if (code != DiscordError::NONE) {
            Gtk::MessageDialog dlg("Failed to set guild icon", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
            dlg.set_position(Gtk::WIN_POS_CENTER);
            dlg.run();
        }
    };
    discord.SetGuildIcon(GuildID, encoded, sigc::track_obj(cb, *this));
}

void GuildSettingsInfoPane::UpdateGuildIconFromPixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf) {
    int w = pixbuf->get_width();
    int h = pixbuf->get_height();
    if (w > 1024 || h > 1024) {
        GetImageDimensions(w, h, w, h, 1024, 1024);
        pixbuf = pixbuf->scale_simple(w, h, Gdk::INTERP_BILINEAR);
    }
    gchar *buffer;
    gsize buffer_size;
    pixbuf->save_to_buffer(buffer, buffer_size, "png");
    std::vector<uint8_t> data(buffer_size);
    std::memcpy(data.data(), buffer, buffer_size);
    UpdateGuildIconFromData(data, "image/png");
}

void GuildSettingsInfoPane::UpdateGuildIconPicker() {
    auto dlg = Gtk::FileChooserNative::create("Choose new guild icon", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg->set_modal(true);
    dlg->signal_response().connect([this, dlg](int response) {
        if (response == Gtk::RESPONSE_ACCEPT) {
            auto data = ReadWholeFile(dlg->get_filename());
            if (GetExtension(dlg->get_filename()) == ".gif")
                UpdateGuildIconFromData(data, "image/gif");
            else
                try {
                    auto loader = Gdk::PixbufLoader::create();
                    loader->signal_size_prepared().connect([&loader](int inw, int inh) {
                        int w, h;
                        GetImageDimensions(inw, inh, w, h, 1024, 1024);
                        loader->set_size(w, h);
                    });
                    loader->write(data.data(), data.size());
                    loader->close();
                    UpdateGuildIconFromPixbuf(loader->get_pixbuf());
                } catch (...) {}
        }
    });

    auto filter_images = Gtk::FileFilter::create();
    if (const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(GuildID); guild.has_value() && guild->HasFeature("ANIMATED_ICON")) {
        filter_images->set_name("Supported images (*.jpg, *.jpeg, *.png, *.gif)");
        filter_images->add_pattern("*.gif");
    } else {
        filter_images->set_name("Supported images (*.jpg, *.jpeg, *.png)");
    }
    filter_images->add_pattern("*.jpg");
    filter_images->add_pattern("*.jpeg");
    filter_images->add_pattern("*.png");
    dlg->add_filter(filter_images);

    auto filter_all = Gtk::FileFilter::create();
    filter_all->set_name("All files (*.*)");
    filter_all->add_pattern("*.*");
    dlg->add_filter(filter_all);

    dlg->run();
}
