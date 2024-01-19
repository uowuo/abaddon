#ifdef WITH_QRLOGIN

// clang-format off

#include "remoteauthdialog.hpp"

#include <gdkmm/pixbufloader.h>
#include <qrcodegen.hpp>

#include "abaddon.hpp"

// clang-format on

RemoteAuthDialog::RemoteAuthDialog(Gtk::Window &parent)
    : Gtk::Dialog("Login with QR Code", parent, true)
    , m_layout(Gtk::ORIENTATION_VERTICAL)
    , m_ok("OK")
    , m_cancel("Cancel")
    , m_bbox(Gtk::ORIENTATION_HORIZONTAL) {
    set_default_size(300, 50);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");

    m_ok.signal_clicked().connect([&]() {
        response(Gtk::RESPONSE_OK);
    });

    m_cancel.signal_clicked().connect([&]() {
        response(Gtk::RESPONSE_CANCEL);
    });

    m_bbox.pack_start(m_ok, Gtk::PACK_SHRINK);
    m_bbox.pack_start(m_cancel, Gtk::PACK_SHRINK);
    m_bbox.set_layout(Gtk::BUTTONBOX_END);

    m_ra.signal_hello().connect(sigc::mem_fun(*this, &RemoteAuthDialog::OnHello));
    m_ra.signal_fingerprint().connect(sigc::mem_fun(*this, &RemoteAuthDialog::OnFingerprint));
    m_ra.signal_pending_ticket().connect(sigc::mem_fun(*this, &RemoteAuthDialog::OnPendingTicket));
    m_ra.signal_pending_login().connect(sigc::mem_fun(*this, &RemoteAuthDialog::OnPendingLogin));
    m_ra.signal_token().connect(sigc::mem_fun(*this, &RemoteAuthDialog::OnToken));
    m_ra.signal_error().connect(sigc::mem_fun(*this, &RemoteAuthDialog::OnError));

    m_ra.Start();

    m_image.set_size_request(256, 256);

    m_status.set_text("Connecting...");
    m_status.set_hexpand(true);
    m_status.set_halign(Gtk::ALIGN_CENTER);

    m_layout.add(m_image);
    m_layout.add(m_status);
    m_layout.add(m_bbox);
    get_content_area()->add(m_layout);

    show_all_children();
}

std::string RemoteAuthDialog::GetToken() {
    return m_token;
}

void RemoteAuthDialog::OnHello() {
    m_status.set_text("Handshaking...");
}

void RemoteAuthDialog::OnFingerprint(const std::string &fingerprint) {
    m_status.set_text("Waiting for mobile device...");

    const auto url = "https://discord.com/ra/" + fingerprint;

    const auto level = qrcodegen::QrCode::Ecc::QUARTILE;
    const auto qr = qrcodegen::QrCode::encodeText(url.c_str(), level);

    int size = qr.getSize();
    const int border = 4;

    const auto module_set = "192 0 255";
    const auto module_clr = "255 255 255";

    std::ostringstream sb;
    sb << "P3\n";
    sb << size + border * 2 << " " << size + border * 2 << " 255\n";
    for (int y = -border; y < size + border; y++) {
        for (int x = -border; x < size + border; x++) {
            if (qr.getModule(x, y)) {
                sb << module_set << "\n";
            } else {
                sb << module_clr << "\n";
            }
        }
    }

    const auto img = sb.str();

    auto loader = Gdk::PixbufLoader::create();
    loader->write(reinterpret_cast<const guint8 *>(img.data()), img.size());
    loader->close();
    const auto pb = loader->get_pixbuf()->scale_simple(256, 256, Gdk::INTERP_NEAREST);

    m_image.property_pixbuf() = pb;
}

void RemoteAuthDialog::OnPendingTicket(Snowflake user_id, const std::string &discriminator, const std::string &avatar_hash, const std::string &username) {
    Glib::ustring name = username;
    if (discriminator != "0") {
        name += "#" + discriminator;
    }
    m_status.set_text("Waiting for confirmation... (" + name + ")");

    if (!avatar_hash.empty()) {
        const auto url = "https://cdn.discordapp.com/avatars/" + std::to_string(user_id) + "/" + avatar_hash + ".png?size=256";
        const auto cb = [this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            m_image.property_pixbuf() = pb->scale_simple(256, 256, Gdk::INTERP_BILINEAR);
        };
        Abaddon::Get().GetImageManager().LoadFromURL(url, sigc::track_obj(cb, *this));
    }
}

void RemoteAuthDialog::OnPendingLogin() {
    m_status.set_text("Logging in!");
}

void RemoteAuthDialog::OnToken(const std::string &token) {
    m_token = token;
    m_ra.Stop();
    response(Gtk::RESPONSE_OK);
}

void RemoteAuthDialog::OnError(const std::string &error) {
    m_ra.Stop();
    Abaddon::Get().ShowConfirm(error, dynamic_cast<Gtk::Window *>(get_toplevel()));
    response(Gtk::RESPONSE_CANCEL);
}

#endif
