#pragma once
#include <gtkmm/box.h>
#include <gtkmm/paned.h>
#include "channellisttree.hpp"
#include "classic/guildlist.hpp"
#include "discord/snowflake.hpp"
#include "state.hpp"

// Contains the actual ChannelListTree and the classic listing if enabled
class ChannelList : public Gtk::HBox {
    // have to proxy public and signals to underlying tree... ew!!!
public:
    ChannelList();

    void UpdateListing();
    void SetActiveChannel(Snowflake id, bool expand_to);

    // channel list should be populated when this is called
    void UseExpansionState(const ExpansionStateRoot &state);
    ExpansionStateRoot GetExpansionState() const;

    void UsePanedHack(Gtk::Paned &paned);

    void SetClassic(bool value);

private:
    void ConnectSignals();

    ChannelListTree m_tree;

    Gtk::ScrolledWindow m_guilds_scroll;
    GuildList m_guilds;

    bool m_is_classic = false;

public:
    using type_signal_action_channel_item_select = sigc::signal<void, Snowflake>;
    using type_signal_action_guild_leave = sigc::signal<void, Snowflake>;
    using type_signal_action_guild_settings = sigc::signal<void, Snowflake>;

#ifdef WITH_LIBHANDY
    using type_signal_action_open_new_tab = sigc::signal<void, Snowflake>;
    type_signal_action_open_new_tab signal_action_open_new_tab();
#endif

#ifdef WITH_VOICE
    using type_signal_action_join_voice_channel = sigc::signal<void, Snowflake>;
    using type_signal_action_disconnect_voice = sigc::signal<void>;

    type_signal_action_join_voice_channel signal_action_join_voice_channel();
    type_signal_action_disconnect_voice signal_action_disconnect_voice();
#endif

    type_signal_action_channel_item_select signal_action_channel_item_select();
    type_signal_action_guild_leave signal_action_guild_leave();
    type_signal_action_guild_settings signal_action_guild_settings();

private:
    type_signal_action_channel_item_select m_signal_action_channel_item_select;
    type_signal_action_guild_leave m_signal_action_guild_leave;
    type_signal_action_guild_settings m_signal_action_guild_settings;

#ifdef WITH_LIBHANDY
    type_signal_action_open_new_tab m_signal_action_open_new_tab;
#endif

#ifdef WITH_VOICE
    type_signal_action_join_voice_channel m_signal_action_join_voice_channel;
    type_signal_action_disconnect_voice m_signal_action_disconnect_voice;
#endif
};
