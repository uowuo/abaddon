#include "remoteauthdialog.hpp"
#include <qrcodegen.hpp>

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

    m_ra.signal_fingerprint().connect(sigc::mem_fun(*this, &RemoteAuthDialog::OnFingerprint));

    m_ra.Start();

    m_image.set_size_request(256, 256);

    m_layout.add(m_image);
    m_layout.add(m_bbox);
    get_content_area()->add(m_layout);

    show_all_children();
}

void RemoteAuthDialog::OnFingerprint(const std::string &fingerprint) {
    const auto url = "https://discord.com/ra/" + fingerprint;

    const auto level = qrcodegen::QrCode::Ecc::QUARTILE;
    const auto qr = qrcodegen::QrCode::encodeText(url.c_str(), level);

    int size = qr.getSize();
    const int border = 4;

    std::ostringstream sb;
    sb << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    sb << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    sb << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 ";
    sb << (size + border * 2) << " " << (size + border * 2) << "\" stroke=\"none\">\n";
    sb << "\t<rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>\n";
    sb << "\t<path d=\"";
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qr.getModule(x, y)) {
                if (x != 0 || y != 0)
                    sb << " ";
                sb << "M" << (x + border) << "," << (y + border) << "h1v1h-1z";
            }
        }
    }
    sb << "\" fill=\"#000000\"/>\n";
    sb << "</svg>\n";

    const auto svg = sb.str();

    auto loader = Gdk::PixbufLoader::create();
    loader->write(reinterpret_cast<const guint8 *>(svg.data()), svg.size());
    loader->close();
    const auto pb = loader->get_pixbuf()->scale_simple(256, 256, Gdk::INTERP_NEAREST);

    m_image.property_pixbuf() = pb;
}

std::string RemoteAuthDialog::GetToken() {
    return m_token;
}
