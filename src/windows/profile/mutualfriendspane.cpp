#include "mutualfriendspane.hpp"

#include "abaddon.hpp"

MutualFriendItem::MutualFriendItem(const UserData &user)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL) {
    get_style_context()->add_class("mutual-friend-item");
    m_name.get_style_context()->add_class("mutual-friend-item-name");
    m_avatar.get_style_context()->add_class("mutual-friend-item-avatar");

    m_avatar.set_margin_end(10);

    auto &img = Abaddon::Get().GetImageManager();
    m_avatar.property_pixbuf() = img.GetPlaceholder(24);
    if (user.HasAnimatedAvatar() && Abaddon::Get().GetSettings().ShowAnimations) {
        auto cb = [this](const Glib::RefPtr<Gdk::PixbufAnimation> &pb) {
            m_avatar.property_pixbuf_animation() = pb;
        };
        img.LoadAnimationFromURL(user.GetAvatarURL("gif", "32"), 24, 24, sigc::track_obj(cb, *this));
    } else {
        auto cb = [this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            m_avatar.property_pixbuf() = pb->scale_simple(24, 24, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL(user.GetAvatarURL("png", "32"), sigc::track_obj(cb, *this));
    }

    m_name.set_markup(user.GetUsernameEscapedBold());

    m_name.set_valign(Gtk::ALIGN_CENTER);
    add(m_avatar);
    add(m_name);
    show_all_children();
}

MutualFriendsPane::MutualFriendsPane(Snowflake id)
    : UserID(id) {
    signal_map().connect(sigc::mem_fun(*this, &MutualFriendsPane::OnMap));
    add(m_list);
    show_all_children();
}

void MutualFriendsPane::OnFetchRelationships(const std::vector<UserData> &users) {
    for (auto child : m_list.get_children())
        delete child;

    for (const auto &user : users) {
        auto *item = Gtk::manage(new MutualFriendItem(user));
        item->show();
        m_list.add(*item);
    }
}

void MutualFriendsPane::OnMap() {
    if (m_requested) return;
    m_requested = true;

    Abaddon::Get().GetDiscordClient().FetchUserRelationships(UserID, sigc::mem_fun(*this, &MutualFriendsPane::OnFetchRelationships));
}
