#include "channellist.hpp"

ChannelList::ChannelList() {
    m_tree.show();
    add(m_tree);
}

void ChannelList::UpdateListing() {
    m_tree.UpdateListing();
}

void ChannelList::SetActiveChannel(Snowflake id, bool expand_to) {
    m_tree.SetActiveChannel(id, expand_to);
}

void ChannelList::UseExpansionState(const ExpansionStateRoot &state) {
    m_tree.UseExpansionState(state);
}

ExpansionStateRoot ChannelList::GetExpansionState() const {
    m_tree.GetExpansionState();
}

void ChannelList::UsePanedHack(Gtk::Paned &paned) {
    m_tree.UsePanedHack(paned);
}

ChannelList::type_signal_action_open_new_tab ChannelList::signal_action_open_new_tab() {
    return m_signal_action_open_new_tab;
}

ChannelList::type_signal_action_join_voice_channel ChannelList::signal_action_join_voice_channel() {
    return m_signal_action_join_voice_channel;
}

ChannelList::type_signal_action_disconnect_voice ChannelList::signal_action_disconnect_voice() {
    return m_signal_action_disconnect_voice;
}

ChannelList::type_signal_action_channel_item_select ChannelList::signal_action_channel_item_select() {
    return m_signal_action_channel_item_select;
}

ChannelList::type_signal_action_guild_leave ChannelList::signal_action_guild_leave() {
    return m_signal_action_guild_leave;
}

ChannelList::type_signal_action_guild_settings ChannelList::signal_action_guild_settings() {
    return m_signal_action_guild_settings;
}
