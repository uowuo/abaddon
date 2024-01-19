#include "profilewindow.hpp"

#include "abaddon.hpp"
#include "util.hpp"

ProfileWindow::ProfileWindow(Snowflake user_id)
    : ID(user_id)
    , m_main(Gtk::ORIENTATION_VERTICAL)
    , m_upper(Gtk::ORIENTATION_HORIZONTAL)
    , m_badges(Gtk::ORIENTATION_HORIZONTAL)
    , m_name_box(Gtk::ORIENTATION_VERTICAL)
    , m_pane_info(user_id)
    , m_pane_guilds(user_id)
    , m_pane_friends(user_id) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto user = *discord.GetUser(ID);

    discord.FetchUserProfile(user_id, sigc::mem_fun(*this, &ProfileWindow::OnFetchProfile));

    set_name("user-profile");
    set_default_size(450, 375);
    set_title(user.GetUsername());
    set_position(Gtk::WIN_POS_CENTER);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");
    get_style_context()->add_class("user-profile-window");
    m_main.get_style_context()->add_class("profile-main-container");
    m_avatar.get_style_context()->add_class("profile-avatar");
    m_displayname.get_style_context()->add_class("profile-username");
    m_username.get_style_context()->add_class("profile-username-nondisplay");
    m_switcher.get_style_context()->add_class("profile-switcher");
    m_stack.get_style_context()->add_class("profile-stack");
    m_badges.get_style_context()->add_class("profile-badges");

    m_scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    m_scroll.set_vexpand(true);
    m_scroll.set_propagate_natural_height(true);

    if (user.HasAvatar()) AddPointerCursor(m_avatar_ev);

    m_avatar_ev.signal_button_release_event().connect([user](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_RELEASE && event->button == GDK_BUTTON_PRIMARY) {
            if (user.HasAnimatedAvatar())
                LaunchBrowser(user.GetAvatarURL("gif", "512"));
            else
                LaunchBrowser(user.GetAvatarURL("png", "512"));
        }
        return false;
    });

    auto &img = Abaddon::Get().GetImageManager();
    m_avatar.property_pixbuf() = img.GetPlaceholder(64);
    auto icon_cb = [this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
        set_icon(pb);
    };
    img.LoadFromURL(user.GetAvatarURL("png", "64"), sigc::track_obj(icon_cb, *this));

    if (Abaddon::Get().GetSettings().ShowAnimations && user.HasAnimatedAvatar()) {
        auto cb = [this](const Glib::RefPtr<Gdk::PixbufAnimation> &pb) {
            m_avatar.property_pixbuf_animation() = pb;
        };
        img.LoadAnimationFromURL(user.GetAvatarURL("gif", "64"), 64, 64, sigc::track_obj(cb, *this));
    } else {
        auto cb = [this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            m_avatar.property_pixbuf() = pb->scale_simple(64, 64, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL(user.GetAvatarURL("png", "64"), sigc::track_obj(cb, *this));
    }

    m_displayname.set_markup(user.GetDisplayNameEscaped());
    m_username.set_label(user.GetUsername());

    m_switcher.set_stack(m_stack);
    m_switcher.set_halign(Gtk::ALIGN_START);
    m_switcher.set_hexpand(true);

    m_stack.add(m_pane_info, "info", "User Info");
    m_stack.add(m_pane_guilds, "guilds", "Mutual Servers");
    m_stack.add(m_pane_friends, "friends", "Mutual Friends");

    m_badges.set_valign(Gtk::ALIGN_CENTER);
    m_badges_scroll.set_hexpand(true);
    m_badges_scroll.set_propagate_natural_width(true);
    m_badges_scroll.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER);

    m_upper.set_halign(Gtk::ALIGN_START);
    m_avatar.set_halign(Gtk::ALIGN_START);
    m_displayname.set_halign(Gtk::ALIGN_START);
    m_username.set_halign(Gtk::ALIGN_START);
    m_avatar_ev.add(m_avatar);
    m_upper.add(m_avatar_ev);
    m_upper.add(m_name_box);
    m_name_box.add(m_displayname);
    m_name_box.add(m_username);
    m_badges_scroll.add(m_badges);
    m_upper.add(m_badges_scroll);
    m_main.add(m_upper);
    m_main.add(m_switcher);
    m_scroll.add(m_stack);
    m_main.add(m_scroll);
    add(m_main);
    show_all_children();
}

void ProfileWindow::OnFetchProfile(const UserProfileData &data) {
    m_pane_info.SetProfile(data);
    m_pane_guilds.SetMutualGuilds(data.MutualGuilds);

    if (data.LegacyUsername.has_value()) {
        m_username.set_tooltip_text("Originally known as " + *data.LegacyUsername);
    }

    for (auto child : m_badges.get_children()) {
        delete child;
    }

    if (!data.User.PublicFlags.has_value()) return;
    const auto x = *data.User.PublicFlags;
    for (uint64_t i = 1; i <= UserData::MaxFlag; i <<= 1) {
        if (!(x & i)) continue;
        const std::string name = UserData::GetFlagName(i);
        if (name == "unknown") continue;
        Glib::RefPtr<Gdk::Pixbuf> pixbuf;
        try {
            if (name == "verifiedbot")
                pixbuf = Gdk::Pixbuf::create_from_file(Abaddon::GetResPath("/checkmark.png"), 24, 24);
            else
                pixbuf = Gdk::Pixbuf::create_from_file(Abaddon::GetResPath("/" + name + ".png"), 24, 24);
        } catch (const Glib::Exception &e) {
            pixbuf = Abaddon::Get().GetImageManager().GetPlaceholder(24);
        }
        if (!pixbuf) continue;
        auto *image = Gtk::manage(new Gtk::Image(pixbuf));
        image->get_style_context()->add_class("profile-badge");
        image->set_tooltip_text(UserData::GetFlagReadableName(i));
        image->show();
        m_badges.add(*image);
    }
}
