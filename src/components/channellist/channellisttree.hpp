#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/treemodelsort.h>
#include <gtkmm/treeview.h>
#include <sigc++/sigc++.h>
#include "discord/discord.hpp"
#include "state.hpp"
#include "cellrendererchannels.hpp"

constexpr static int GuildIconSize = 24;
constexpr static int DMIconSize = 20;
constexpr static int VoiceParticipantIconSize = 18;
constexpr static int OrphanChannelSortOffset = -100; // forces orphan channels to the top of the list

class ChannelListTree : public Gtk::ScrolledWindow {
public:
    ChannelListTree();

    void UpdateListing();
    void SetActiveChannel(Snowflake id, bool expand_to);

    // channel list should be populated when this is called
    void UseExpansionState(const ExpansionStateRoot &state);
    ExpansionStateRoot GetExpansionState() const;

    void UsePanedHack(Gtk::Paned &paned);

    void SetClassic(bool value);
    void SetSelectedGuild(Snowflake guild_id);
    void SetSelectedDMs();

protected:
    int SortFunc(const Gtk::TreeModel::iterator &a, const Gtk::TreeModel::iterator &b);

    void OnPanedPositionChanged();

    void UpdateListingClassic();

    void UpdateNewGuild(const GuildData &guild);
    void UpdateRemoveGuild(Snowflake id);
    void UpdateRemoveChannel(Snowflake id);
    void UpdateChannel(Snowflake id);
    void UpdateCreateChannel(const ChannelData &channel);
    void UpdateGuild(Snowflake id);
    void DeleteThreadRow(Snowflake id);
    void OnChannelMute(Snowflake id);
    void OnChannelUnmute(Snowflake id);
    void OnGuildMute(Snowflake id);
    void OnGuildUnmute(Snowflake id);

    void OnThreadJoined(Snowflake id);
    void OnThreadRemoved(Snowflake id);
    void OnThreadDelete(const ThreadDeleteData &data);
    void OnThreadUpdate(const ThreadUpdateData &data);
    void OnThreadListSync(const ThreadListSyncData &data);

    void OnVoiceUserConnect(Snowflake user_id, Snowflake channel_id);
    void OnVoiceUserDisconnect(Snowflake user_id, Snowflake channel_id);
    void OnVoiceStateSet(Snowflake user_id, Snowflake channel_id, VoiceStateFlags flags);

    Gtk::TreeView m_view;

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns();

        Gtk::TreeModelColumn<RenderType> m_type;
        Gtk::TreeModelColumn<uint64_t> m_id;
        Gtk::TreeModelColumn<Glib::ustring> m_name;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> m_icon;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::PixbufAnimation>> m_icon_anim;
        Gtk::TreeModelColumn<int64_t> m_sort;
        Gtk::TreeModelColumn<bool> m_nsfw;
        Gtk::TreeModelColumn<std::optional<Gdk::RGBA>> m_color; // for folders right now
        Gtk::TreeModelColumn<VoiceStateFlags> m_voice_flags;
        // Gtk::CellRenderer's property_is_expanded only works how i want it to if it has children
        // because otherwise it doesnt count as an "expander" (property_is_expander)
        // so this solution will have to do which i hate but the alternative is adding invisible children
        // to all categories without children and having a filter model but that sounds worse
        // of course its a lot better than the absolute travesty i had before
        Gtk::TreeModelColumn<bool> m_expanded;
    };

    ModelColumns m_columns;
    Glib::RefPtr<Gtk::TreeStore> m_model;
    Glib::RefPtr<Gtk::TreeModelFilter> m_filter_model;
    Glib::RefPtr<Gtk::TreeModelSort> m_sort_model;

    Gtk::TreePath ConvertModelPathToView(const Gtk::TreePath &path);
    Gtk::TreeIter ConvertModelIterToView(const Gtk::TreeIter &iter);
    Gtk::TreePath ConvertViewPathToModel(const Gtk::TreePath &path);
    Gtk::TreeIter ConvertViewIterToModel(const Gtk::TreeIter &iter);
    Gtk::TreePath GetViewPathFromViewIter(const Gtk::TreeIter &iter);
    Gtk::TreeModel::iterator AddFolder(const UserSettingsGuildFoldersEntry &folder);
    Gtk::TreeModel::iterator AddGuild(const GuildData &guild, const Gtk::TreeNodeChildren &root);
    Gtk::TreeModel::iterator UpdateCreateChannelCategory(const ChannelData &channel);
    Gtk::TreeModel::iterator CreateThreadRow(const Gtk::TreeNodeChildren &children, const ChannelData &channel);

#ifdef WITH_VOICE
    Gtk::TreeModel::iterator CreateVoiceParticipantRow(const UserData &user, const Gtk::TreeNodeChildren &parent);
#endif

    void UpdateChannelCategory(const ChannelData &channel);

    // separation necessary because a channel and guild can share the same id
    Gtk::TreeModel::iterator GetIteratorForTopLevelFromID(Snowflake id);
    Gtk::TreeModel::iterator GetIteratorForGuildFromID(Snowflake id);
    Gtk::TreeModel::iterator GetIteratorForRowFromID(Snowflake id);
    Gtk::TreeModel::iterator GetIteratorForRowFromIDOfType(Snowflake id, RenderType type);

    bool IsTextChannel(ChannelType type);

    void OnRowCollapsed(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path) const;
    void OnRowExpanded(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path);
    bool SelectionFunc(const Glib::RefPtr<Gtk::TreeModel> &model, const Gtk::TreeModel::Path &path, bool is_currently_selected);
    bool OnButtonPressEvent(GdkEventButton *ev);

    void MoveRow(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::iterator &new_parent);

    Gtk::TreeModel::Path m_last_selected;
    Gtk::TreeModel::Path m_dm_header;

    void AddPrivateChannels();
    void UpdateCreateDMChannel(const ChannelData &channel);
    void SetDMChannelIcon(Gtk::TreeIter iter, const ChannelData &dm);

    void RedrawUnreadIndicatorsForChannel(const ChannelData &channel);
    void OnMessageAck(const MessageAckData &data);
    void OnMessageCreate(const Message &msg);

    Gtk::TreeModel::Path m_path_for_menu;

    // cant be recovered through selection
    Gtk::TreeModel::iterator m_temporary_thread_row;

    Gtk::Menu m_menu_guild;
    Gtk::MenuItem m_menu_guild_copy_id;
    Gtk::MenuItem m_menu_guild_settings;
    Gtk::MenuItem m_menu_guild_leave;
    Gtk::MenuItem m_menu_guild_mark_as_read;
    Gtk::MenuItem m_menu_guild_toggle_mute;

    Gtk::Menu m_menu_category;
    Gtk::MenuItem m_menu_category_copy_id;
    Gtk::MenuItem m_menu_category_toggle_mute;

    Gtk::Menu m_menu_channel;
    Gtk::MenuItem m_menu_channel_copy_id;
    Gtk::MenuItem m_menu_channel_mark_as_read;
    Gtk::MenuItem m_menu_channel_toggle_mute;

#ifdef WITH_LIBHANDY
    Gtk::MenuItem m_menu_channel_open_tab;
#endif

#ifdef WITH_VOICE
    Gtk::Menu m_menu_voice_channel;
    Gtk::MenuItem m_menu_voice_channel_join;
    Gtk::MenuItem m_menu_voice_channel_disconnect;
#endif

    Gtk::Menu m_menu_dm;
    Gtk::MenuItem m_menu_dm_copy_id;
    Gtk::MenuItem m_menu_dm_close;
    Gtk::MenuItem m_menu_dm_toggle_mute;
#ifdef WITH_VOICE
    Gtk::MenuItem m_menu_dm_join_voice;
    Gtk::MenuItem m_menu_dm_disconnect_voice;
#endif

#ifdef WITH_LIBHANDY
    Gtk::MenuItem m_menu_dm_open_tab;
#endif

    Gtk::Menu m_menu_thread;
    Gtk::MenuItem m_menu_thread_copy_id;
    Gtk::MenuItem m_menu_thread_leave;
    Gtk::MenuItem m_menu_thread_archive;
    Gtk::MenuItem m_menu_thread_unarchive;
    Gtk::MenuItem m_menu_thread_mark_as_read;
    Gtk::MenuItem m_menu_thread_toggle_mute;

    void OnGuildSubmenuPopup();
    void OnCategorySubmenuPopup();
    void OnChannelSubmenuPopup();
    void OnDMSubmenuPopup();
    void OnThreadSubmenuPopup();

#ifdef WITH_VOICE
    void OnVoiceChannelSubmenuPopup();
#endif

    bool m_updating_listing = false;

    bool m_classic = false;
    Snowflake m_classic_selected_guild;
    bool m_classic_selected_dms = false;

    Snowflake m_active_channel;

    // hashtable for the billion lookups done in UseExpansionState
    std::unordered_map<Snowflake, Gtk::TreeModel::iterator> m_tmp_row_map;
    std::unordered_map<Snowflake, Gtk::TreeModel::iterator> m_tmp_guild_row_map;

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
