#include "call.hpp"
#include "abaddon.hpp"

CallDialog::CallDialog(Gtk::Window &parent, Snowflake channel_id, bool is_group_call)
    : Gtk::Dialog(is_group_call ? "Incoming Group Call" : "Incoming Call", parent, true)
    , m_channel_id(channel_id)
    , m_is_group_call(is_group_call)
    , m_main_layout(Gtk::ORIENTATION_VERTICAL)
    , m_info_layout(Gtk::ORIENTATION_HORIZONTAL)
    , m_button_box(Gtk::ORIENTATION_HORIZONTAL)
    , m_accept_button(is_group_call ? "Join" : "Accept")
    , m_reject_button("Reject") {
    set_default_size(350, 150);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");

    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto channel = discord.GetChannel(channel_id);

    if (!channel.has_value()) {
        m_title_label.set_text("Unknown Call");
        m_info_label.set_text("Channel information unavailable");
    } else if (m_is_group_call) {
        m_title_label.set_markup("<b>Group Call</b>");
        m_info_label.set_text(channel->GetRecipientsDisplay());
        m_avatar.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(64);
    } else {
        const auto recipients = channel->GetDMRecipients();
        if (recipients.empty()) {
            m_title_label.set_text("Unknown Call");
            m_info_label.set_text("User information unavailable");
            m_avatar.property_pixbuf() = Abaddon::Get().GetImageManager().GetPlaceholder(64);
        } else {
            const auto &user = recipients[0];
            m_title_label.set_markup("<b>" + Glib::Markup::escape_text(user.GetDisplayName()) + "</b>");
            m_info_label.set_text("is calling you");

            auto &img = Abaddon::Get().GetImageManager();
            m_avatar.property_pixbuf() = img.GetPlaceholder(64);

            if (user.HasAnimatedAvatar() && Abaddon::Get().GetSettings().ShowAnimations) {
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
        }
    }

    m_avatar.set_margin_end(10);
    m_avatar.set_halign(Gtk::ALIGN_START);
    m_avatar.set_valign(Gtk::ALIGN_CENTER);

    m_title_label.set_halign(Gtk::ALIGN_START);
    m_title_label.set_valign(Gtk::ALIGN_CENTER);
    m_info_label.set_halign(Gtk::ALIGN_START);
    m_info_label.set_valign(Gtk::ALIGN_CENTER);
    m_info_label.set_single_line_mode(true);
    m_info_label.set_ellipsize(Pango::ELLIPSIZE_END);

    m_accept_button.signal_clicked().connect(sigc::mem_fun(*this, &CallDialog::OnAccept));
    m_reject_button.signal_clicked().connect(sigc::mem_fun(*this, &CallDialog::OnReject));

    m_button_box.pack_start(m_accept_button, Gtk::PACK_SHRINK);
    m_button_box.pack_start(m_reject_button, Gtk::PACK_SHRINK);
    m_button_box.set_layout(Gtk::BUTTONBOX_END);
    m_button_box.set_margin_top(10);

    Gtk::Box *title_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    title_box->add(m_title_label);
    title_box->add(m_info_label);
    title_box->set_halign(Gtk::ALIGN_START);
    title_box->set_valign(Gtk::ALIGN_CENTER);

    m_info_layout.add(m_avatar);
    m_info_layout.add(*title_box);
    m_info_layout.set_margin_bottom(10);

    m_main_layout.add(m_info_layout);
    m_main_layout.add(m_button_box);

    get_content_area()->add(m_main_layout);

    signal_response().connect([this](int response_id) {
        if (response_id != Gtk::RESPONSE_OK) {
            m_accepted = false;
        }
    });

    show_all_children();
}

bool CallDialog::GetAccepted() const {
    return m_accepted;
}

void CallDialog::OnAccept() {
    m_accepted = true;
    response(Gtk::RESPONSE_OK);
}

void CallDialog::OnReject() {
    m_accepted = false;
    response(Gtk::RESPONSE_CANCEL);
}
