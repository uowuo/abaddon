#pragma once
#include <gtkmm/dialog.h>
#include "remoteauthclient.hpp"

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
    void OnPendingTicket(Snowflake user_id, std::string discriminator, std::string avatar_hash, std::string username);
    void OnPendingLogin();
    void OnToken(const std::string &token);
    void OnError(const std::string &error);

    std::string m_token;
};
