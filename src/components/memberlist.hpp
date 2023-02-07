#pragma once
#include <mutex>
#include <unordered_map>
#include <optional>
#include "discord/discord.hpp"

class LazyImage;
class StatusIndicator;
class MemberListUserRow : public Gtk::ListBoxRow {
public:
    MemberListUserRow(const std::optional<GuildData> &guild, const UserData &data);

    Snowflake ID;

private:
    Gtk::EventBox *m_ev;
    Gtk::Box *m_box;
    LazyImage *m_avatar;
    StatusIndicator *m_status_indicator;
    Gtk::Label *m_label;
    Gtk::Image *m_crown = nullptr;
};

class MemberList {
public:
    MemberList();
    Gtk::Widget *GetRoot() const;

    void UpdateMemberList();
    void Clear();
    void SetActiveChannel(Snowflake id);

private:
    void AttachUserMenuHandler(Gtk::ListBoxRow *row, Snowflake id);

    Gtk::ScrolledWindow *m_main;
    Gtk::ListBox *m_listbox;

    Snowflake m_guild_id;
    Snowflake m_chan_id;

    std::unordered_map<Snowflake, Gtk::ListBoxRow *> m_id_to_row;
};
