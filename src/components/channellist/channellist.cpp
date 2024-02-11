#include "channellist.hpp"

#include "abaddon.hpp"

ChannelList::ChannelList() {
    get_style_context()->add_class("channel-browser-pane");

    ConnectSignals();

    m_guilds.set_halign(Gtk::ALIGN_START);

    m_guilds_scroll.get_style_context()->add_class("guild-list-scroll");
    m_guilds_scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    m_guilds.signal_guild_selected().connect([this](Snowflake guild_id) {
        m_tree.SetSelectedGuild(guild_id);
    });

    m_guilds.signal_dms_selected().connect([this]() {
        m_tree.SetSelectedDMs();
    });

    m_guilds.show();
    m_tree.show();
    m_guilds_scroll.add(m_guilds);
    pack_start(m_guilds_scroll, false, false); // only take the space it needs
    pack_start(m_tree, true, true);            // use all the remaining space
}

void ChannelList::UpdateListing() {
    m_tree.UpdateListing();
    if (m_is_classic) m_guilds.UpdateListing();
}

void ChannelList::SetActiveChannel(Snowflake id, bool expand_to) {
    if (Abaddon::Get().GetSettings().ClassicChangeGuildOnOpen) {
        if (const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(id); channel.has_value() && channel->GuildID.has_value()) {
            m_tree.SetSelectedGuild(*channel->GuildID);
        } else {
            m_tree.SetSelectedDMs();
        }
    }

    m_tree.SetActiveChannel(id, expand_to);
}

void ChannelList::UseExpansionState(const ExpansionStateRoot &state) {
    m_tree.UseExpansionState(state);
}

ExpansionStateRoot ChannelList::GetExpansionState() const {
    return m_tree.GetExpansionState();
}

void ChannelList::UsePanedHack(Gtk::Paned &paned) {
    m_tree.UsePanedHack(paned);
}

void ChannelList::SetClassic(bool value) {
    m_is_classic = value;
    m_tree.SetClassic(value);
    m_guilds_scroll.set_visible(value);
}

void ChannelList::ConnectSignals() {
    // TODO: if these all just travel upwards to the singleton then get rid of them but mayeb they dont

#ifdef WITH_LIBHANDY
    m_tree.signal_action_open_new_tab().connect([this](Snowflake id) {
        m_signal_action_open_new_tab.emit(id);
    });
#endif

#ifdef WITH_VOICE
    m_tree.signal_action_join_voice_channel().connect([this](Snowflake id) {
        m_signal_action_join_voice_channel.emit(id);
    });

    m_tree.signal_action_disconnect_voice().connect([this]() {
        m_signal_action_disconnect_voice.emit();
    });
#endif

    m_tree.signal_action_channel_item_select().connect([this](Snowflake id) {
        m_signal_action_channel_item_select.emit(id);
    });

    m_tree.signal_action_guild_leave().connect([this](Snowflake id) {
        m_signal_action_guild_leave.emit(id);
    });

    m_tree.signal_action_guild_settings().connect([this](Snowflake id) {
        m_signal_action_guild_settings.emit(id);
    });

    m_guilds.signal_action_guild_leave().connect([this](Snowflake id) {
        m_signal_action_guild_leave.emit(id);
    });

    m_guilds.signal_action_guild_settings().connect([this](Snowflake id) {
        m_signal_action_guild_settings.emit(id);
    });
}

#ifdef WITH_LIBHANDY
ChannelList::type_signal_action_open_new_tab ChannelList::signal_action_open_new_tab() {
    return m_signal_action_open_new_tab;
}
#endif

#ifdef WITH_VOICE
ChannelList::type_signal_action_join_voice_channel ChannelList::signal_action_join_voice_channel() {
    return m_signal_action_join_voice_channel;
}

ChannelList::type_signal_action_disconnect_voice ChannelList::signal_action_disconnect_voice() {
    return m_signal_action_disconnect_voice;
}
#endif

ChannelList::type_signal_action_channel_item_select ChannelList::signal_action_channel_item_select() {
    return m_signal_action_channel_item_select;
}

ChannelList::type_signal_action_guild_leave ChannelList::signal_action_guild_leave() {
    return m_signal_action_guild_leave;
}

ChannelList::type_signal_action_guild_settings ChannelList::signal_action_guild_settings() {
    return m_signal_action_guild_settings;
}
