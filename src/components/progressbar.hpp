#pragma once
#include <gtkmm/progressbar.h>
#include <string>

class MessageUploadProgressBar : public Gtk::ProgressBar {
public:
    MessageUploadProgressBar();

private:
    std::string m_last_nonce;
};
