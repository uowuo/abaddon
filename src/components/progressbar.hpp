#pragma once
#include <string>
#include <gtkmm/progressbar.h>

class MessageUploadProgressBar : public Gtk::ProgressBar {
public:
    MessageUploadProgressBar();

private:
    bool m_active = false;
    std::string m_last_nonce;

    using type_signal_start = sigc::signal<void()>;
    using type_signal_stop = sigc::signal<void()>;

    type_signal_start m_signal_start;
    type_signal_stop m_signal_stop;

public:
    type_signal_start signal_start();
    type_signal_stop signal_stop();
};
