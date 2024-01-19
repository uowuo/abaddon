#include "emojispane.hpp"

#include <gtkmm/messagedialog.h>
#include <gtkmm/treemodelfilter.h>

#include "abaddon.hpp"
#include "components/cellrendererpixbufanimation.hpp"
#include "util.hpp"

GuildSettingsEmojisPane::GuildSettingsEmojisPane(Snowflake guild_id)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , GuildID(guild_id)
    , m_model(Gtk::ListStore::create(m_columns))
    , m_filter(Gtk::TreeModelFilter::create(m_model))
    , m_menu_delete("Delete")
    , m_menu_copy_id("Copy ID")
    , m_menu_copy_emoji_url("Copy Emoji URL")
    , m_menu_show_emoji("Open in Browser") {
    signal_map().connect(sigc::mem_fun(*this, &GuildSettingsEmojisPane::OnMap));
    set_name("guild-emojis-pane");

    m_view_scroll.set_hexpand(true);
    m_view_scroll.set_vexpand(true);

    m_view.signal_button_press_event().connect(sigc::mem_fun(*this, &GuildSettingsEmojisPane::OnTreeButtonPress), false);

    m_menu_copy_id.signal_activate().connect(sigc::mem_fun(*this, &GuildSettingsEmojisPane::OnMenuCopyID));
    m_menu_delete.signal_activate().connect(sigc::mem_fun(*this, &GuildSettingsEmojisPane::OnMenuDelete));
    m_menu_copy_emoji_url.signal_activate().connect(sigc::mem_fun(*this, &GuildSettingsEmojisPane::OnMenuCopyEmojiURL));
    m_menu_show_emoji.signal_activate().connect(sigc::mem_fun(*this, &GuildSettingsEmojisPane::OnMenuShowEmoji));

    m_menu.append(m_menu_delete);
    m_menu.append(m_menu_copy_id);
    m_menu.append(m_menu_copy_emoji_url);
    m_menu.append(m_menu_show_emoji);
    m_menu.show_all();

    auto &discord = Abaddon::Get().GetDiscordClient();

    discord.signal_guild_emojis_update().connect(sigc::hide<0>(sigc::mem_fun(*this, &GuildSettingsEmojisPane::OnFetchEmojis)));

    const auto self_id = discord.GetUserData().ID;
    const bool can_manage = discord.HasGuildPermission(self_id, GuildID, Permission::MANAGE_GUILD_EXPRESSIONS);
    m_menu_delete.set_sensitive(can_manage);

    m_search.set_placeholder_text("Filter");
    m_search.signal_changed().connect([this]() {
        m_filter->refilter();
    });

    m_view_scroll.add(m_view);
    add(m_search);
    add(m_view_scroll);
    m_search.show();
    m_view.show();
    m_view_scroll.show();

    m_filter->set_visible_func([this](const Gtk::TreeModel::const_iterator &iter) -> bool {
        const auto text = m_search.get_text();
        if (text.empty()) return true;
        return StringContainsCaseless((*iter)[m_columns.m_col_name], text);
    });
    m_view.set_enable_search(false);
    m_view.set_model(m_filter);

    auto *column = Gtk::manage(new Gtk::TreeView::Column("Emoji"));
    auto *renderer = Gtk::manage(new CellRendererPixbufAnimation);
    column->pack_start(*renderer);
    column->add_attribute(renderer->property_pixbuf(), m_columns.m_col_pixbuf);
    column->add_attribute(renderer->property_pixbuf_animation(), m_columns.m_col_pixbuf_animation);
    m_view.append_column(*column);

    if (can_manage) {
        auto *column = Gtk::manage(new Gtk::TreeView::Column("Name"));
        auto *renderer = Gtk::manage(new Gtk::CellRendererText);
        column->pack_start(*renderer);
        column->add_attribute(renderer->property_text(), m_columns.m_col_name);
        renderer->property_editable() = true;
        renderer->signal_edited().connect([this](const Glib::ustring &path, const Glib::ustring &text) {
            std::string new_str;
            int size = 0;
            for (const auto ch : text) {
                if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')
                    new_str += static_cast<char>(ch);
                else if (ch == ' ')
                    new_str += '_';
                if (++size == 32) break;
            }
            if (auto row = *m_model->get_iter(path)) {
                row[m_columns.m_col_name] = new_str;
                OnEditName(row[m_columns.m_col_id], new_str);
            }
        });
        m_view.append_column(*column);
    } else
        m_view.append_column("Name", m_columns.m_col_name);
    if (can_manage)
        m_view.append_column("Creator", m_columns.m_col_creator);
    m_view.append_column("Is Animated?", m_columns.m_col_animated);

    for (const auto column : m_view.get_columns())
        column->set_resizable(true);
}

void GuildSettingsEmojisPane::OnMap() {
    m_view.grab_focus();

    if (m_requested) return;
    m_requested = true;

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto self_id = discord.GetUserData().ID;
    const bool can_manage = discord.HasGuildPermission(self_id, GuildID, Permission::MANAGE_GUILD_EXPRESSIONS);
    m_menu_delete.set_sensitive(can_manage);

    discord.FetchGuildEmojis(GuildID, sigc::mem_fun(*this, &GuildSettingsEmojisPane::OnFetchEmojis));
}

void GuildSettingsEmojisPane::AddEmojiRow(const EmojiData &emoji) {
    auto &img = Abaddon::Get().GetImageManager();

    auto row = *m_model->append();

    row[m_columns.m_col_id] = emoji.ID;
    row[m_columns.m_col_pixbuf] = img.GetPlaceholder(32);
    row[m_columns.m_col_name] = emoji.Name;
    if (emoji.Creator.has_value())
        row[m_columns.m_col_creator] = emoji.Creator->GetUsername();
    if (emoji.IsAnimated.has_value())
        row[m_columns.m_col_animated] = *emoji.IsAnimated ? "Yes" : "No";
    else
        row[m_columns.m_col_animated] = "No";
    if (emoji.IsAvailable.has_value())
        row[m_columns.m_col_available] = *emoji.IsAvailable ? "Yes" : "No";
    else
        row[m_columns.m_col_available] = "Yes";

    if (Abaddon::Get().GetSettings().ShowAnimations && emoji.IsAnimated.has_value() && *emoji.IsAnimated) {
        const auto cb = [this, id = emoji.ID](const Glib::RefPtr<Gdk::PixbufAnimation> &pb) {
            for (auto &row : m_model->children()) {
                if (static_cast<Snowflake>(row[m_columns.m_col_id]) == id) {
                    row[m_columns.m_col_pixbuf_animation] = pb;
                    return;
                }
            }
        };
        img.LoadAnimationFromURL(emoji.GetURL("gif"), 32, 32, sigc::track_obj(cb, *this));
    } else {
        const auto cb = [this, id = emoji.ID](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            for (auto &row : m_model->children()) {
                if (static_cast<Snowflake>(row[m_columns.m_col_id]) == id) {
                    row[m_columns.m_col_pixbuf] = pb->scale_simple(32, 32, Gdk::INTERP_BILINEAR);
                    return;
                }
            }
        };
        img.LoadFromURL(emoji.GetURL(), sigc::track_obj(cb, *this));
    }
}

void GuildSettingsEmojisPane::OnFetchEmojis(std::vector<EmojiData> emojis) {
    m_model->clear();

    // put animated emojis at the end then sort alphabetically
    std::sort(emojis.begin(), emojis.end(), [&](const EmojiData &a, const EmojiData &b) {
        const bool a_is_animated = a.IsAnimated.has_value() && *a.IsAnimated;
        const bool b_is_animated = b.IsAnimated.has_value() && *b.IsAnimated;
        if (a_is_animated == b_is_animated)
            return a.Name < b.Name;
        else if (a_is_animated && !b_is_animated)
            return false;
        else if (!a_is_animated && b_is_animated)
            return true;
        return false; // this wont happen please be quiet compiler
    });

    for (const auto &emoji : emojis)
        AddEmojiRow(emoji);
}

void GuildSettingsEmojisPane::OnEditName(Snowflake id, const std::string &name) {
    const auto cb = [](DiscordError code) {
        if (code != DiscordError::NONE) {
            Gtk::MessageDialog dlg("Failed to set emoji name", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
            dlg.set_position(Gtk::WIN_POS_CENTER);
            dlg.run();
        }
    };
    Abaddon::Get().GetDiscordClient().ModifyEmojiName(GuildID, id, name, cb);
}

void GuildSettingsEmojisPane::OnMenuCopyID() {
    if (auto selected_row = *m_view.get_selection()->get_selected()) {
        const auto id = static_cast<Snowflake>(selected_row[m_columns.m_col_id]);
        Gtk::Clipboard::get()->set_text(std::to_string(id));
    }
}

void GuildSettingsEmojisPane::OnMenuDelete() {
    if (auto selected_row = *m_view.get_selection()->get_selected()) {
        const auto name = static_cast<Glib::ustring>(selected_row[m_columns.m_col_name]);
        const auto id = static_cast<Snowflake>(selected_row[m_columns.m_col_id]);
        if (auto *window = dynamic_cast<Gtk::Window *>(get_toplevel()))
            if (Abaddon::Get().ShowConfirm("Are you sure you want to delete " + name + "?", window)) {
                const auto cb = [](DiscordError code) {
                    if (code != DiscordError::NONE) {
                        Gtk::MessageDialog dlg("Failed to delete emoji", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
                        dlg.set_position(Gtk::WIN_POS_CENTER);
                        dlg.run();
                    }
                };
                Abaddon::Get().GetDiscordClient().DeleteEmoji(GuildID, id, cb);
            }
    }
}

void GuildSettingsEmojisPane::OnMenuCopyEmojiURL() {
    if (auto selected_row = *m_view.get_selection()->get_selected()) {
        const auto id = static_cast<Snowflake>(selected_row[m_columns.m_col_id]);
        const bool is_animated = static_cast<Glib::ustring>(selected_row[m_columns.m_col_animated]) == "Yes";
        Gtk::Clipboard::get()->set_text(EmojiData::URLFromID(id, is_animated ? "gif" : "png", "256"));
    }
}

void GuildSettingsEmojisPane::OnMenuShowEmoji() {
    if (auto selected_row = *m_view.get_selection()->get_selected()) {
        const auto id = static_cast<Snowflake>(selected_row[m_columns.m_col_id]);
        const bool is_animated = static_cast<Glib::ustring>(selected_row[m_columns.m_col_animated]) == "Yes";
        LaunchBrowser(EmojiData::URLFromID(id, is_animated ? "gif" : "png", "256"));
    }
}

bool GuildSettingsEmojisPane::OnTreeButtonPress(GdkEventButton *event) {
    if (event->button == GDK_BUTTON_SECONDARY) {
        auto &discord = Abaddon::Get().GetDiscordClient();
        const auto self_id = discord.GetUserData().ID;
        const bool can_manage = discord.HasGuildPermission(self_id, GuildID, Permission::MANAGE_GUILD_EXPRESSIONS);
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

GuildSettingsEmojisPane::ModelColumns::ModelColumns() {
    add(m_col_id);
    add(m_col_pixbuf);
    add(m_col_pixbuf_animation);
    add(m_col_name);
    add(m_col_creator);
    add(m_col_animated);
    add(m_col_available);
}
