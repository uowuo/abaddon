#include "friendpicker.hpp"

#include "abaddon.hpp"

FriendPickerDialog::FriendPickerDialog(Gtk::Window &parent)
    : Gtk::Dialog("Pick a friend", parent, true)
    , m_bbox(Gtk::ORIENTATION_HORIZONTAL) {
    set_default_size(300, 300);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");

    m_ok_button = add_button("OK", Gtk::RESPONSE_OK);
    m_cancel_button = add_button("Cancel", Gtk::RESPONSE_CANCEL);

    m_ok_button->set_sensitive(false);

    auto &discord = Abaddon::Get().GetDiscordClient();
    auto relationships = discord.GetRelationships(RelationshipType::Friend);
    for (auto id : relationships) {
        auto *item = Gtk::manage(new FriendPickerDialogItem(id));
        item->show();
        m_list.add(*item);
    }

    m_list.signal_row_activated().connect(sigc::mem_fun(*this, &FriendPickerDialog::OnRowActivated));
    m_list.signal_selected_rows_changed().connect(sigc::mem_fun(*this, &FriendPickerDialog::OnSelectionChange));
    m_list.set_activate_on_single_click(false);
    m_list.set_selection_mode(Gtk::SELECTION_SINGLE);

    m_main.set_propagate_natural_height(true);

    m_main.add(m_list);

    get_content_area()->add(m_main);

    show_all_children();
}

Snowflake FriendPickerDialog::GetUserID() const {
    return m_chosen_id;
}

void FriendPickerDialog::OnRowActivated(Gtk::ListBoxRow *row) {
    auto *x = dynamic_cast<FriendPickerDialogItem *>(row);
    if (x != nullptr) {
        m_chosen_id = x->ID;
        response(Gtk::RESPONSE_OK);
    }
}

void FriendPickerDialog::OnSelectionChange() {
    auto selection = m_list.get_selected_row();
    m_ok_button->set_sensitive(false);
    if (selection != nullptr) {
        auto *row = dynamic_cast<FriendPickerDialogItem *>(selection);
        if (!row) return;
        m_chosen_id = row->ID;
        m_ok_button->set_sensitive(true);
    }
}

FriendPickerDialogItem::FriendPickerDialogItem(Snowflake user_id)
    : ID(user_id)
    , m_layout(Gtk::ORIENTATION_HORIZONTAL) {
    auto user = *Abaddon::Get().GetDiscordClient().GetUser(user_id);

    m_name.set_markup(user.GetUsernameEscapedBold());
    m_name.set_single_line_mode(true);

    m_avatar.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(32);
    if (user.HasAnimatedAvatar() && Abaddon::Get().GetSettings().ShowAnimations) {
        auto cb = [this](const Glib::RefPtr<Gdk::PixbufAnimation> &pb) {
            m_avatar.property_pixbuf_animation() = pb;
        };
        Abaddon::Get().GetImageManager().LoadAnimationFromURL(user.GetAvatarURL("gif", "32"), 32, 32, sigc::track_obj(cb, *this));
    } else {
        auto cb = [this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            m_avatar.property_pixbuf() = pb->scale_simple(32, 32, Gdk::INTERP_BILINEAR);
        };
        Abaddon::Get().GetImageManager().LoadFromURL(user.GetAvatarURL("png", "32"), sigc::track_obj(cb, *this));
    }

    m_avatar.set_margin_end(5);
    m_avatar.set_halign(Gtk::ALIGN_START);
    m_avatar.set_valign(Gtk::ALIGN_CENTER);
    m_name.set_halign(Gtk::ALIGN_START);
    m_name.set_valign(Gtk::ALIGN_CENTER);

    m_layout.add(m_avatar);
    m_layout.add(m_name);
    add(m_layout);

    show_all_children();
}
