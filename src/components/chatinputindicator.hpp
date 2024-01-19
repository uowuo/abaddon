#pragma once
#include <unordered_map>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include "discord/message.hpp"
#include "discord/user.hpp"

class ChatInputIndicator : public Gtk::Box {
public:
    ChatInputIndicator();
    void SetActiveChannel(Snowflake id);
    void SetCustomMarkup(const Glib::ustring &str);
    void ClearCustom();

private:
    void AddUser(Snowflake channel_id, const UserData &user, int timeout);
    void OnUserTypingStart(Snowflake user_id, Snowflake channel_id);
    void OnMessageCreate(const Message &message);
    void SetTypingString(const Glib::ustring &str);
    void ComputeTypingString();

    Gtk::Image m_img;
    Gtk::Label m_label;

    Glib::ustring m_custom_markup;

    Snowflake m_active_channel;
    std::optional<Snowflake> m_active_guild;
    std::unordered_map<Snowflake, std::unordered_map<Snowflake, sigc::connection>> m_typers; // channel id -> [user id -> connection]
};
