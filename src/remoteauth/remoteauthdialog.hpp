#pragma once

#ifdef WITH_QRLOGIN

// clang-format off

#include <gtkmm/dialog.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

#include "remoteauthclient.hpp"

// clang-format on

class RemoteAuthDialog : public Gtk::Dialog {
public:
    RemoteAuthDialog(Gtk::Window &parent);
    std::string GetToken();

protected:
    Gtk::Image m_image;
    Gtk::Label m_status;
    Gtk::Box m_layout;
    Gtk::Button m_ok;
    Gtk::Button m_cancel;
    Gtk::ButtonBox m_bbox;

private:
    RemoteAuthClient m_ra;

    void OnHello();
    void OnFingerprint(const std::string &fingerprint);
    void OnPendingTicket(Snowflake user_id, const std::string &discriminator, const std::string &avatar_hash, const std::string &username);
    void OnPendingLogin();
    void OnToken(const std::string &token);
    void OnError(const std::string &error);

    std::string m_token;
};

#endif
