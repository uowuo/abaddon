#pragma once
#include <gtkmm.h>
#include <unordered_map>
#include "../discord/snowflake.hpp"
#include "../discord/user.hpp"

class TypingIndicator : public Gtk::Box {
public:
    TypingIndicator();
    void SetActiveChannel(Snowflake id);

private:
    void AddUser(Snowflake channel_id, const UserData &user, int timeout);
    void OnUserTypingStart(Snowflake user_id, Snowflake channel_id);
    void OnMessageCreate(Snowflake message_id);
    void SetTypingString(const Glib::ustring &str);
    void ComputeTypingString();

    Gtk::Image m_img;
    Gtk::Label m_label;

    Snowflake m_active_channel;
    std::unordered_map<Snowflake, std::unordered_map<Snowflake, sigc::connection>> m_typers; // channel id -> [user id -> connection]
};
