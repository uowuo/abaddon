#pragma once
#include <gtkmm.h>
#include <string>
#include <chrono>

class JoinGuildDialog : public Gtk::Dialog {
public:
    JoinGuildDialog(Gtk::Window &parent);
    std::string GetCode();

protected:
    void on_entry_changed();
    static bool IsCode(std::string str);

    Gtk::Box m_layout;
    Gtk::Button m_ok;
    Gtk::Button m_cancel;
    Gtk::Box m_lower;
    Gtk::Label m_info;
    Gtk::Entry m_entry;

    void CheckCode();

    // needs a rate limit cuz if u hit it u get ip banned from /invites for a long time :(
    bool m_needs_request = false;
    std::chrono::time_point<std::chrono::steady_clock> m_last_req_time;
    bool on_idle_slot();

private:
    std::string m_code;
};
