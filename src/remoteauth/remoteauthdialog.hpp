#pragma once
#include <gtkmm/dialog.h>
#include "remoteauthclient.hpp"

class RemoteAuthDialog : public Gtk::Dialog {
public:
    RemoteAuthDialog(Gtk::Window &parent);
    std::string GetToken();

protected:
    Gtk::Image m_image;
    Gtk::Box m_layout;
    Gtk::Button m_ok;
    Gtk::Button m_cancel;
    Gtk::ButtonBox m_bbox;

private:
    RemoteAuthClient m_ra;

    void OnFingerprint(const std::string &fingerprint);

    std::string m_token;
};
