#include "channellisttree.hpp"

#include <algorithm>
#include <map>
#include <unordered_map>

#include <gtkmm/main.h>

#include "abaddon.hpp"
#include "imgmanager.hpp"
#include "util.hpp"

ChannelListTree::ChannelListTree()
    : Glib::ObjectBase(typeid(ChannelListTree))
    , m_model(Gtk::TreeStore::create(m_columns))
    , m_filter_model(Gtk::TreeModelFilter::create(m_model))
    , m_sort_model(Gtk::TreeModelSort::create(m_filter_model))
    , m_menu_guild_copy_id("_Copy ID", true)
    , m_menu_guild_settings("View _Settings", true)
    , m_menu_guild_leave("_Leave", true)
    , m_menu_guild_mark_as_read("Mark as _Read", true)
    , m_menu_category_copy_id("_Copy ID", true)
    , m_menu_channel_copy_id("_Copy ID", true)
    , m_menu_channel_mark_as_read("Mark as _Read", true)
#ifdef WITH_LIBHANDY
    , m_menu_channel_open_tab("Open in New _Tab", true)
    , m_menu_dm_open_tab("Open in New _Tab", true)
#endif
#ifdef WITH_VOICE
    , m_menu_voice_channel_join("_Join", true)
    , m_menu_voice_channel_disconnect("_Disconnect", true)
#endif
    , m_menu_dm_copy_id("_Copy ID", true)
    , m_menu_dm_close("") // changes depending on if group or not
#ifdef WITH_VOICE
    , m_menu_dm_join_voice("Join _Voice", true)
    , m_menu_dm_disconnect_voice("_Disconnect Voice", true)
#endif
    , m_menu_thread_copy_id("_Copy ID", true)
    , m_menu_thread_leave("_Leave", true)
    , m_menu_thread_archive("_Archive", true)
    , m_menu_thread_unarchive("_Unarchive", true)
    , m_menu_thread_mark_as_read("Mark as _Read", true) {
    get_style_context()->add_class("channel-list");

    // Filter iters
    const auto cb = [this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column) {
        auto view_path = ConvertViewPathToModel(path);
        if (!view_path) return;
        auto row = *m_model->get_iter(view_path);
        if (!row) return;
        const auto type = row[m_columns.m_type];
        // text channels should not be allowed to be collapsed
        // maybe they should be but it seems a little difficult to handle expansion to permit this
        if (type != RenderType::TextChannel && type != RenderType::DM) {
            if (row[m_columns.m_expanded]) {
                m_view.collapse_row(path);
                row[m_columns.m_expanded] = false;
            } else {
                m_view.expand_row(path, false);
                row[m_columns.m_expanded] = true;
            }
        }

        if (type == RenderType::TextChannel || type == RenderType::DM || type == RenderType::Thread) {
            const auto id = static_cast<Snowflake>(row[m_columns.m_id]);
            m_signal_action_channel_item_select.emit(id);
            Abaddon::Get().GetDiscordClient().MarkChannelAsRead(id, [](...) {});
        }
    };
    m_view.signal_row_activated().connect(cb, false);
    m_view.signal_row_collapsed().connect(sigc::mem_fun(*this, &ChannelListTree::OnRowCollapsed), false);
    m_view.signal_row_expanded().connect(sigc::mem_fun(*this, &ChannelListTree::OnRowExpanded), false);
    m_view.set_activate_on_single_click(true);
    m_view.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
    m_view.get_selection()->set_select_function(sigc::mem_fun(*this, &ChannelListTree::SelectionFunc));
    m_view.signal_button_press_event().connect(sigc::mem_fun(*this, &ChannelListTree::OnButtonPressEvent), false);

    m_view.set_hexpand(true);
    m_view.set_vexpand(true);

    m_view.set_show_expanders(false);
    m_view.set_enable_search(false);
    m_view.set_headers_visible(false);
    m_view.set_model(m_sort_model);
    m_sort_model->set_sort_column(m_columns.m_sort, Gtk::SORT_ASCENDING);
    m_sort_model->set_sort_func(m_columns.m_sort, sigc::mem_fun(*this, &ChannelListTree::SortFunc));

    m_model->signal_row_inserted().connect([this](const Gtk::TreeModel::Path &path, const Gtk::TreeModel::iterator &iter) {
        if (m_updating_listing) return;
        if (auto parent = iter->parent(); parent && (*parent)[m_columns.m_expanded]) {
            if (const auto view_path = ConvertModelPathToView(m_model->get_path(parent))) {
                m_view.expand_row(view_path, false);
            }
        }
    });

    m_filter_model->set_visible_func([this](const Gtk::TreeModel::const_iterator &iter) -> bool {
        if (!m_classic || m_updating_listing) return true;

        const RenderType type = (*iter)[m_columns.m_type];

        if (m_classic_selected_dms) {
            if (iter->parent()) return true;
            return type == RenderType::DMHeader;
        }

        if (type == RenderType::Guild) {
            return (*iter)[m_columns.m_id] == m_classic_selected_guild;
        }
        return type != RenderType::DMHeader;
    });

    m_view.show();

    add(m_view);

    auto *column = Gtk::manage(new Gtk::TreeView::Column("display"));
    auto *renderer = Gtk::manage(new CellRendererChannels);
    column->pack_start(*renderer);
    column->add_attribute(renderer->property_type(), m_columns.m_type);
    column->add_attribute(renderer->property_icon(), m_columns.m_icon);
    column->add_attribute(renderer->property_icon_animation(), m_columns.m_icon_anim);
    column->add_attribute(renderer->property_name(), m_columns.m_name);
    column->add_attribute(renderer->property_id(), m_columns.m_id);
    column->add_attribute(renderer->property_expanded(), m_columns.m_expanded);
    column->add_attribute(renderer->property_nsfw(), m_columns.m_nsfw);
    column->add_attribute(renderer->property_color(), m_columns.m_color);
    column->add_attribute(renderer->property_voice_state(), m_columns.m_voice_flags);
    m_view.append_column(*column);

    m_menu_guild_copy_id.signal_activate().connect([this] {
        Gtk::Clipboard::get()->set_text(std::to_string((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });
    m_menu_guild_settings.signal_activate().connect([this] {
        m_signal_action_guild_settings.emit(static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });
    m_menu_guild_leave.signal_activate().connect([this] {
        m_signal_action_guild_leave.emit(static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });
    m_menu_guild_mark_as_read.signal_activate().connect([this] {
        Abaddon::Get().GetDiscordClient().MarkGuildAsRead(static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]), [](...) {});
    });
    m_menu_guild_toggle_mute.signal_activate().connect([this] {
        const auto id = static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]);
        auto &discord = Abaddon::Get().GetDiscordClient();
        if (discord.IsGuildMuted(id))
            discord.UnmuteGuild(id, NOOP_CALLBACK);
        else
            discord.MuteGuild(id, NOOP_CALLBACK);
    });
    m_menu_guild.append(m_menu_guild_mark_as_read);
    m_menu_guild.append(m_menu_guild_settings);
    m_menu_guild.append(m_menu_guild_leave);
    m_menu_guild.append(m_menu_guild_toggle_mute);
    m_menu_guild.append(m_menu_guild_copy_id);
    m_menu_guild.show_all();

    m_menu_category_copy_id.signal_activate().connect([this] {
        Gtk::Clipboard::get()->set_text(std::to_string((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });
    m_menu_category_toggle_mute.signal_activate().connect([this] {
        const auto id = static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]);
        auto &discord = Abaddon::Get().GetDiscordClient();
        if (discord.IsChannelMuted(id))
            discord.UnmuteChannel(id, NOOP_CALLBACK);
        else
            discord.MuteChannel(id, NOOP_CALLBACK);
    });
    m_menu_category.append(m_menu_category_toggle_mute);
    m_menu_category.append(m_menu_category_copy_id);
    m_menu_category.show_all();

    m_menu_channel_copy_id.signal_activate().connect([this] {
        Gtk::Clipboard::get()->set_text(std::to_string((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });
    m_menu_channel_mark_as_read.signal_activate().connect([this] {
        Abaddon::Get().GetDiscordClient().MarkChannelAsRead(static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]), [](...) {});
    });
    m_menu_channel_toggle_mute.signal_activate().connect([this] {
        const auto id = static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]);
        auto &discord = Abaddon::Get().GetDiscordClient();
        if (discord.IsChannelMuted(id))
            discord.UnmuteChannel(id, NOOP_CALLBACK);
        else
            discord.MuteChannel(id, NOOP_CALLBACK);
    });

#ifdef WITH_LIBHANDY
    m_menu_channel_open_tab.signal_activate().connect([this] {
        const auto id = static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]);
        m_signal_action_open_new_tab.emit(id);
    });
    m_menu_channel.append(m_menu_channel_open_tab);
#endif

    m_menu_channel.append(m_menu_channel_mark_as_read);
    m_menu_channel.append(m_menu_channel_toggle_mute);
    m_menu_channel.append(m_menu_channel_copy_id);
    m_menu_channel.show_all();

#ifdef WITH_VOICE
    m_menu_voice_channel_join.signal_activate().connect([this]() {
        const auto id = static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]);
        m_signal_action_join_voice_channel.emit(id);
    });

    m_menu_voice_channel_disconnect.signal_activate().connect([this]() {
        m_signal_action_disconnect_voice.emit();
    });

    m_menu_voice_channel.append(m_menu_voice_channel_join);
    m_menu_voice_channel.append(m_menu_voice_channel_disconnect);
    m_menu_voice_channel.show_all();
#endif

    m_menu_dm_copy_id.signal_activate().connect([this] {
        Gtk::Clipboard::get()->set_text(std::to_string((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });
    m_menu_dm_close.signal_activate().connect([this] {
        const auto id = static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]);
        auto &discord = Abaddon::Get().GetDiscordClient();
        const auto channel = discord.GetChannel(id);
        if (!channel.has_value()) return;

        if (channel->Type == ChannelType::DM)
            discord.CloseDM(id);
        else if (Abaddon::Get().ShowConfirm("Are you sure you want to leave this group DM?"))
            Abaddon::Get().GetDiscordClient().CloseDM(id);
    });
    m_menu_dm_toggle_mute.signal_activate().connect([this] {
        const auto id = static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]);
        auto &discord = Abaddon::Get().GetDiscordClient();
        if (discord.IsChannelMuted(id))
            discord.UnmuteChannel(id, NOOP_CALLBACK);
        else
            discord.MuteChannel(id, NOOP_CALLBACK);
    });
#ifdef WITH_LIBHANDY
    m_menu_dm_open_tab.signal_activate().connect([this] {
        const auto id = static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]);
        m_signal_action_open_new_tab.emit(id);
    });
    m_menu_dm.append(m_menu_dm_open_tab);
#endif
    m_menu_dm.append(m_menu_dm_toggle_mute);
    m_menu_dm.append(m_menu_dm_close);
#ifdef WITH_VOICE
    m_menu_dm_join_voice.signal_activate().connect([this]() {
        const auto id = static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]);
        m_signal_action_join_voice_channel.emit(id);
    });
    m_menu_dm_disconnect_voice.signal_activate().connect([this]() {
        m_signal_action_disconnect_voice.emit();
    });
    m_menu_dm.append(m_menu_dm_join_voice);
    m_menu_dm.append(m_menu_dm_disconnect_voice);
#endif
    m_menu_dm.append(m_menu_dm_copy_id);
    m_menu_dm.show_all();

    m_menu_thread_copy_id.signal_activate().connect([this] {
        Gtk::Clipboard::get()->set_text(std::to_string((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });
    m_menu_thread_leave.signal_activate().connect([this] {
        if (Abaddon::Get().ShowConfirm("Are you sure you want to leave this thread?"))
            Abaddon::Get().GetDiscordClient().LeaveThread(static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]), "Context%20Menu", [](...) {});
    });
    m_menu_thread_archive.signal_activate().connect([this] {
        Abaddon::Get().GetDiscordClient().ArchiveThread(static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]), [](...) {});
    });
    m_menu_thread_unarchive.signal_activate().connect([this] {
        Abaddon::Get().GetDiscordClient().UnArchiveThread(static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]), [](...) {});
    });
    m_menu_thread_mark_as_read.signal_activate().connect([this] {
        Abaddon::Get().GetDiscordClient().MarkChannelAsRead(static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]), NOOP_CALLBACK);
    });
    m_menu_thread_toggle_mute.signal_activate().connect([this] {
        const auto id = static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]);
        auto &discord = Abaddon::Get().GetDiscordClient();
        if (discord.IsChannelMuted(id))
            discord.UnmuteThread(id, NOOP_CALLBACK);
        else
            discord.MuteThread(id, NOOP_CALLBACK);
    });
    m_menu_thread.append(m_menu_thread_mark_as_read);
    m_menu_thread.append(m_menu_thread_toggle_mute);
    m_menu_thread.append(m_menu_thread_leave);
    m_menu_thread.append(m_menu_thread_archive);
    m_menu_thread.append(m_menu_thread_unarchive);
    m_menu_thread.append(m_menu_thread_copy_id);
    m_menu_thread.show_all();

    auto &discord = Abaddon::Get().GetDiscordClient();
    discord.signal_message_create().connect(sigc::mem_fun(*this, &ChannelListTree::OnMessageCreate));
    discord.signal_guild_create().connect(sigc::mem_fun(*this, &ChannelListTree::UpdateNewGuild));
    discord.signal_guild_delete().connect(sigc::mem_fun(*this, &ChannelListTree::UpdateRemoveGuild));
    discord.signal_channel_delete().connect(sigc::mem_fun(*this, &ChannelListTree::UpdateRemoveChannel));
    discord.signal_channel_update().connect(sigc::mem_fun(*this, &ChannelListTree::UpdateChannel));
    discord.signal_channel_create().connect(sigc::mem_fun(*this, &ChannelListTree::UpdateCreateChannel));
    discord.signal_thread_delete().connect(sigc::mem_fun(*this, &ChannelListTree::OnThreadDelete));
    discord.signal_thread_update().connect(sigc::mem_fun(*this, &ChannelListTree::OnThreadUpdate));
    discord.signal_thread_list_sync().connect(sigc::mem_fun(*this, &ChannelListTree::OnThreadListSync));
    discord.signal_added_to_thread().connect(sigc::mem_fun(*this, &ChannelListTree::OnThreadJoined));
    discord.signal_removed_from_thread().connect(sigc::mem_fun(*this, &ChannelListTree::OnThreadRemoved));
    discord.signal_guild_update().connect(sigc::mem_fun(*this, &ChannelListTree::UpdateGuild));
    discord.signal_message_ack().connect(sigc::mem_fun(*this, &ChannelListTree::OnMessageAck));
    discord.signal_channel_muted().connect(sigc::mem_fun(*this, &ChannelListTree::OnChannelMute));
    discord.signal_channel_unmuted().connect(sigc::mem_fun(*this, &ChannelListTree::OnChannelUnmute));
    discord.signal_guild_muted().connect(sigc::mem_fun(*this, &ChannelListTree::OnGuildMute));
    discord.signal_guild_unmuted().connect(sigc::mem_fun(*this, &ChannelListTree::OnGuildUnmute));

#if WITH_VOICE
    discord.signal_voice_user_connect().connect(sigc::mem_fun(*this, &ChannelListTree::OnVoiceUserConnect));
    discord.signal_voice_user_disconnect().connect(sigc::mem_fun(*this, &ChannelListTree::OnVoiceUserDisconnect));
    discord.signal_voice_state_set().connect(sigc::mem_fun(*this, &ChannelListTree::OnVoiceStateSet));
#endif
}

void ChannelListTree::UsePanedHack(Gtk::Paned &paned) {
    paned.property_position().signal_changed().connect(sigc::mem_fun(*this, &ChannelListTree::OnPanedPositionChanged));
}

void ChannelListTree::SetClassic(bool value) {
    m_classic = value;
    m_filter_model->refilter();
}

void ChannelListTree::SetSelectedGuild(Snowflake guild_id) {
    m_classic_selected_guild = guild_id;
    m_classic_selected_dms = false;
    m_filter_model->refilter();
    auto guild_iter = GetIteratorForGuildFromID(guild_id);
    if (guild_iter) {
        if (auto view_iter = ConvertModelIterToView(guild_iter)) {
            m_view.expand_row(GetViewPathFromViewIter(view_iter), false);
        }
    }
}

void ChannelListTree::SetSelectedDMs() {
    m_classic_selected_dms = true;
    m_filter_model->refilter();
    if (m_dm_header) {
        if (auto view_path = ConvertModelPathToView(m_dm_header)) {
            m_view.expand_row(view_path, false);
        }
    }
}

int ChannelListTree::SortFunc(const Gtk::TreeModel::iterator &a, const Gtk::TreeModel::iterator &b) {
    const RenderType a_type = (*a)[m_columns.m_type];
    const RenderType b_type = (*b)[m_columns.m_type];
    const int64_t a_sort = (*a)[m_columns.m_sort];
    const int64_t b_sort = (*b)[m_columns.m_sort];
    if (a_type == RenderType::DMHeader) return -1;
    if (b_type == RenderType::DMHeader) return 1;
#ifdef WITH_VOICE
    if (a_type == RenderType::TextChannel && b_type == RenderType::VoiceChannel) return -1;
    if (b_type == RenderType::TextChannel && a_type == RenderType::VoiceChannel) return 1;
#endif
    return static_cast<int>(std::clamp(a_sort - b_sort, int64_t(-1), int64_t(1)));
}

void ChannelListTree::OnPanedPositionChanged() {
    m_view.queue_draw();
}

void ChannelListTree::UpdateListingClassic() {
    m_updating_listing = true;

    // refilter so every row is visible
    // otherwise clear() causes a CRITICAL assert in a slot for the filter model
    m_filter_model->refilter();
    m_model->clear();

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto guild_ids = discord.GetUserSortedGuilds();
    for (const auto guild_id : guild_ids) {
        if (const auto guild = discord.GetGuild(guild_id); guild.has_value()) {
            AddGuild(*guild, m_model->children());
        }
    }

    m_updating_listing = false;

    AddPrivateChannels();
}

void ChannelListTree::UpdateListing() {
    if (m_classic) {
        UpdateListingClassic();
        return;
    }

    m_updating_listing = true;

    m_model->clear();

    auto &discord = Abaddon::Get().GetDiscordClient();

    /*
    guild_folders looks something like this
     "guild_folders": [
        {
           "color": null,
           "guild_ids": [
               "8009060___________"
           ],
           "id": null,
           "name": null
        },
        {
           "color": null,
           "guild_ids": [
               "99615594__________",
               "86132141__________",
               "35450138__________",
               "83714048__________"
           ],
           "id": 2853066769,
           "name": null
        }
    ]

     so if id != null then its a folder (they can have single entries)
    */

    int sort_value = 0;

    const auto folders = discord.GetUserSettings().GuildFolders;
    const auto guild_ids = discord.GetUserSortedGuilds();

    // user_settings.guild_folders may not contain every guild the user is in
    // this seems to be the case if you organize your guilds and join a server without further organization
    // so add guilds not present in guild_folders by descending id order first

    std::set<Snowflake> foldered_guilds;
    for (const auto &group : folders) {
        foldered_guilds.insert(group.GuildIDs.begin(), group.GuildIDs.end());
    }

    for (auto iter = guild_ids.rbegin(); iter != guild_ids.rend(); iter++) {
        if (foldered_guilds.find(*iter) == foldered_guilds.end()) {
            const auto guild = discord.GetGuild(*iter);
            if (!guild.has_value()) continue;
            auto tree_iter = AddGuild(*guild, m_model->children());
            if (tree_iter) (*tree_iter)[m_columns.m_sort] = sort_value++;
        }
    }

    // then whatever is in folders

    for (const auto &group : folders) {
        auto iter = AddFolder(group);
        if (iter) (*iter)[m_columns.m_sort] = sort_value++;
    }

    m_updating_listing = false;

    AddPrivateChannels();
}

// TODO update for folders
void ChannelListTree::UpdateNewGuild(const GuildData &guild) {
    AddGuild(guild, m_model->children());
    // update sort order
    int sortnum = 0;
    for (const auto guild_id : Abaddon::Get().GetDiscordClient().GetUserSortedGuilds()) {
        auto iter = GetIteratorForGuildFromID(guild_id);
        if (iter)
            (*iter)[m_columns.m_sort] = ++sortnum;
    }
}

void ChannelListTree::UpdateRemoveGuild(Snowflake id) {
    auto iter = GetIteratorForGuildFromID(id);
    if (!iter) return;
    m_model->erase(iter);
}

void ChannelListTree::UpdateRemoveChannel(Snowflake id) {
    auto iter = GetIteratorForRowFromID(id);
    if (!iter) return;
    m_model->erase(iter);
}

void ChannelListTree::UpdateChannel(Snowflake id) {
    auto iter = GetIteratorForRowFromID(id);
    auto channel = Abaddon::Get().GetDiscordClient().GetChannel(id);
    if (!iter || !channel.has_value()) return;
    if (channel->Type == ChannelType::GUILD_CATEGORY) return UpdateChannelCategory(*channel);
    if (!channel->IsText()) return;

    // refresh stuff that might have changed
    const bool is_orphan_TMP = !channel->ParentID.has_value();
    (*iter)[m_columns.m_name] = "#" + Glib::Markup::escape_text(*channel->Name);
    (*iter)[m_columns.m_nsfw] = channel->NSFW();
    (*iter)[m_columns.m_sort] = *channel->Position + (is_orphan_TMP ? OrphanChannelSortOffset : 0);

    // check if the parent has changed
    Gtk::TreeModel::iterator new_parent;
    if (channel->ParentID.has_value())
        new_parent = GetIteratorForRowFromID(*channel->ParentID);
    else if (channel->GuildID.has_value())
        new_parent = GetIteratorForGuildFromID(*channel->GuildID);

    if (new_parent && iter->parent() != new_parent)
        MoveRow(iter, new_parent);
}

void ChannelListTree::UpdateCreateChannel(const ChannelData &channel) {
    if (channel.Type == ChannelType::GUILD_CATEGORY) return (void)UpdateCreateChannelCategory(channel);
    if (channel.Type == ChannelType::DM || channel.Type == ChannelType::GROUP_DM) return UpdateCreateDMChannel(channel);
    if (channel.Type != ChannelType::GUILD_TEXT && channel.Type != ChannelType::GUILD_NEWS) return;

    Gtk::TreeRow channel_row;
    bool orphan;
    if (channel.ParentID.has_value()) {
        orphan = false;
        auto iter = GetIteratorForRowFromID(*channel.ParentID);
        channel_row = *m_model->append(iter->children());
    } else {
        orphan = true;
        auto iter = GetIteratorForGuildFromID(*channel.GuildID);
        channel_row = *m_model->append(iter->children());
    }
    channel_row[m_columns.m_type] = RenderType::TextChannel;
    channel_row[m_columns.m_id] = channel.ID;
    channel_row[m_columns.m_name] = "#" + Glib::Markup::escape_text(*channel.Name);
    channel_row[m_columns.m_nsfw] = channel.NSFW();
    if (orphan)
        channel_row[m_columns.m_sort] = *channel.Position + OrphanChannelSortOffset;
    else
        channel_row[m_columns.m_sort] = *channel.Position;
}

void ChannelListTree::UpdateGuild(Snowflake id) {
    auto iter = GetIteratorForGuildFromID(id);
    auto &img = Abaddon::Get().GetImageManager();
    const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(id);
    if (!iter || !guild.has_value()) return;

    (*iter)[m_columns.m_name] = "<b>" + Glib::Markup::escape_text(guild->Name) + "</b>";
    (*iter)[m_columns.m_icon] = img.GetPlaceholder(GuildIconSize);
    if (Abaddon::Get().GetSettings().ShowAnimations && guild->HasAnimatedIcon()) {
        const auto cb = [this, id](const Glib::RefPtr<Gdk::PixbufAnimation> &pb) {
            auto iter = GetIteratorForGuildFromID(id);
            if (iter) (*iter)[m_columns.m_icon_anim] = pb;
        };
        img.LoadAnimationFromURL(guild->GetIconURL("gif", "32"), GuildIconSize, GuildIconSize, sigc::track_obj(cb, *this));
    } else if (guild->HasIcon()) {
        const auto cb = [this, id](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            // iter might be invalid
            auto iter = GetIteratorForGuildFromID(id);
            if (iter) (*iter)[m_columns.m_icon] = pb->scale_simple(GuildIconSize, GuildIconSize, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL(guild->GetIconURL("png", "32"), sigc::track_obj(cb, *this));
    }
}

void ChannelListTree::OnThreadJoined(Snowflake id) {
    if (GetIteratorForRowFromID(id)) return;
    const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(id);
    if (!channel.has_value()) return;
    const auto parent = GetIteratorForRowFromID(*channel->ParentID);
    if (parent)
        CreateThreadRow(parent->children(), *channel);
}

void ChannelListTree::OnThreadRemoved(Snowflake id) {
    DeleteThreadRow(id);
}

void ChannelListTree::OnThreadDelete(const ThreadDeleteData &data) {
    DeleteThreadRow(data.ID);
}

// todo probably make the row stick around if its selected until the selection changes
void ChannelListTree::OnThreadUpdate(const ThreadUpdateData &data) {
    auto iter = GetIteratorForRowFromID(data.Thread.ID);
    if (iter)
        (*iter)[m_columns.m_name] = "- " + Glib::Markup::escape_text(*data.Thread.Name);

    if (data.Thread.ThreadMetadata->IsArchived)
        DeleteThreadRow(data.Thread.ID);
}

void ChannelListTree::OnThreadListSync(const ThreadListSyncData &data) {
    // get the threads in the guild
    std::vector<Snowflake> threads;
    auto guild_iter = GetIteratorForGuildFromID(data.GuildID);
    if (!guild_iter) return;

    std::queue<Gtk::TreeModel::iterator> queue;
    queue.push(guild_iter);

    while (!queue.empty()) {
        auto item = queue.front();
        queue.pop();
        if ((*item)[m_columns.m_type] == RenderType::Thread)
            threads.push_back(static_cast<Snowflake>((*item)[m_columns.m_id]));
        for (const auto &child : item->children())
            queue.push(child);
    }

    // delete all threads not present in the synced data
    for (auto thread_id : threads) {
        if (std::find_if(data.Threads.begin(), data.Threads.end(), [thread_id](const auto &x) { return x.ID == thread_id; }) == data.Threads.end()) {
            auto iter = GetIteratorForRowFromID(thread_id);
            m_model->erase(iter);
        }
    }

    // delete all archived threads
    for (auto thread : data.Threads) {
        if (thread.ThreadMetadata->IsArchived) {
            if (auto iter = GetIteratorForRowFromID(thread.ID))
                m_model->erase(iter);
        }
    }
}

#ifdef WITH_VOICE
void ChannelListTree::OnVoiceUserConnect(Snowflake user_id, Snowflake channel_id) {
    auto parent_iter = GetIteratorForRowFromIDOfType(channel_id, RenderType::VoiceChannel);
    if (!parent_iter) parent_iter = GetIteratorForRowFromIDOfType(channel_id, RenderType::DM);
    if (!parent_iter) return;
    const auto user = Abaddon::Get().GetDiscordClient().GetUser(user_id);
    if (!user.has_value()) return;

    CreateVoiceParticipantRow(*user, parent_iter->children());
}

void ChannelListTree::OnVoiceUserDisconnect(Snowflake user_id, Snowflake channel_id) {
    if (auto iter = GetIteratorForRowFromIDOfType(user_id, RenderType::VoiceParticipant)) {
        m_model->erase(iter);
    }
}

void ChannelListTree::OnVoiceStateSet(Snowflake user_id, Snowflake channel_id, VoiceStateFlags flags) {
    if (auto iter = GetIteratorForRowFromIDOfType(user_id, RenderType::VoiceParticipant)) {
        (*iter)[m_columns.m_voice_flags] = flags;
    }
}
#endif

void ChannelListTree::DeleteThreadRow(Snowflake id) {
    auto iter = GetIteratorForRowFromID(id);
    if (iter)
        m_model->erase(iter);
}

void ChannelListTree::OnChannelMute(Snowflake id) {
    if (auto iter = GetIteratorForRowFromID(id))
        m_model->row_changed(m_model->get_path(iter), iter);
}

void ChannelListTree::OnChannelUnmute(Snowflake id) {
    if (auto iter = GetIteratorForRowFromID(id))
        m_model->row_changed(m_model->get_path(iter), iter);
}

void ChannelListTree::OnGuildMute(Snowflake id) {
    if (auto iter = GetIteratorForGuildFromID(id))
        m_model->row_changed(m_model->get_path(iter), iter);
}

void ChannelListTree::OnGuildUnmute(Snowflake id) {
    if (auto iter = GetIteratorForGuildFromID(id))
        m_model->row_changed(m_model->get_path(iter), iter);
}

// create a temporary channel row for non-joined threads
// and delete them when the active channel switches off of them if still not joined
void ChannelListTree::SetActiveChannel(Snowflake id, bool expand_to) {
    while (Gtk::Main::events_pending()) Gtk::Main::iteration();

    // mark channel as read when switching off
    if (m_active_channel.IsValid())
        Abaddon::Get().GetDiscordClient().MarkChannelAsRead(m_active_channel, [](...) {});

    m_active_channel = id;

    if (m_temporary_thread_row) {
        const auto thread_id = static_cast<Snowflake>((*m_temporary_thread_row)[m_columns.m_id]);
        const auto thread = Abaddon::Get().GetDiscordClient().GetChannel(thread_id);
        if (thread.has_value() && (!thread->IsJoinedThread() || thread->ThreadMetadata->IsArchived))
            m_model->erase(m_temporary_thread_row);
        m_temporary_thread_row = {};
    }

    const auto channel_iter = GetIteratorForRowFromID(id);
    if (channel_iter) {
        m_view.get_selection()->unselect_all();
        const auto view_iter = ConvertModelIterToView(channel_iter);
        if (view_iter) {
            if (expand_to) {
                m_view.expand_to_path(GetViewPathFromViewIter(view_iter));
            }
            m_view.get_selection()->select(view_iter);
        }
    } else {
        m_view.get_selection()->unselect_all();
        const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(id);
        if (!channel.has_value() || !channel->IsThread()) return;
        auto parent_iter = GetIteratorForRowFromID(*channel->ParentID);
        if (!parent_iter) return;
        m_temporary_thread_row = CreateThreadRow(parent_iter->children(), *channel);
        const auto view_iter = ConvertModelIterToView(m_temporary_thread_row);
        if (view_iter) {
            m_view.get_selection()->select(view_iter);
        }
    }
}

void ChannelListTree::UseExpansionState(const ExpansionStateRoot &root) {
    m_updating_listing = true;
    m_filter_model->refilter();

    auto recurse = [this](auto &self, const ExpansionStateRoot &root) -> void {
        for (const auto &[id, state] : root.Children) {
            Gtk::TreeModel::iterator row_iter;
            if (const auto map_iter = m_tmp_row_map.find(id); map_iter != m_tmp_row_map.end()) {
                row_iter = map_iter->second;
            } else if (const auto map_iter = m_tmp_guild_row_map.find(id); map_iter != m_tmp_guild_row_map.end()) {
                row_iter = map_iter->second;
            }

            if (row_iter) {
                (*row_iter)[m_columns.m_expanded] = state.IsExpanded;
                auto view_iter = ConvertModelIterToView(row_iter);
                if (view_iter) {
                    if (state.IsExpanded) {
                        m_view.expand_row(GetViewPathFromViewIter(view_iter), false);
                    } else {
                        m_view.collapse_row(GetViewPathFromViewIter(view_iter));
                    }
                }
            }

            self(self, state.Children);
        }
    };

    for (const auto &[id, state] : root.Children) {
        if (const auto iter = GetIteratorForTopLevelFromID(id)) {
            (*iter)[m_columns.m_expanded] = state.IsExpanded;
            auto view_iter = ConvertModelIterToView(iter);
            if (view_iter) {
                if (state.IsExpanded) {
                    m_view.expand_row(GetViewPathFromViewIter(view_iter), false);
                } else {
                    m_view.collapse_row(GetViewPathFromViewIter(view_iter));
                }
            }
        }

        recurse(recurse, state.Children);
    }

    m_updating_listing = false;
    m_filter_model->refilter();

    m_tmp_row_map.clear();
    m_tmp_guild_row_map.clear();
}

ExpansionStateRoot ChannelListTree::GetExpansionState() const {
    ExpansionStateRoot r;

    auto recurse = [this](auto &self, const Gtk::TreeRow &row) -> ExpansionState {
        ExpansionState r;

        r.IsExpanded = row[m_columns.m_expanded];
        for (auto child : row.children()) {
            r.Children.Children[static_cast<Snowflake>(child[m_columns.m_id])] = self(self, child);
        }

        return r;
    };

    for (auto child : m_model->children()) {
        const auto id = static_cast<Snowflake>(child[m_columns.m_id]);
        if (static_cast<uint64_t>(id) == 0ULL) continue; // dont save DM header
        r.Children[id] = recurse(recurse, child);
    }

    return r;
}

Gtk::TreePath ChannelListTree::ConvertModelPathToView(const Gtk::TreePath &path) {
    if (const auto filter_path = m_filter_model->convert_child_path_to_path(path)) {
        if (const auto sort_path = m_sort_model->convert_child_path_to_path(filter_path)) {
            return sort_path;
        }
    }

    return {};
}

Gtk::TreeIter ChannelListTree::ConvertModelIterToView(const Gtk::TreeIter &iter) {
    if (const auto filter_iter = m_filter_model->convert_child_iter_to_iter(iter)) {
        if (const auto sort_iter = m_sort_model->convert_child_iter_to_iter(filter_iter)) {
            return sort_iter;
        }
    }

    return {};
}

Gtk::TreePath ChannelListTree::ConvertViewPathToModel(const Gtk::TreePath &path) {
    if (const auto filter_path = m_sort_model->convert_path_to_child_path(path)) {
        if (const auto model_path = m_filter_model->convert_path_to_child_path(filter_path)) {
            return model_path;
        }
    }

    return {};
}

Gtk::TreeIter ChannelListTree::ConvertViewIterToModel(const Gtk::TreeIter &iter) {
    if (const auto filter_iter = m_sort_model->convert_iter_to_child_iter(iter)) {
        if (const auto model_iter = m_filter_model->convert_iter_to_child_iter(filter_iter)) {
            return model_iter;
        }
    }

    return {};
}

Gtk::TreePath ChannelListTree::GetViewPathFromViewIter(const Gtk::TreeIter &iter) {
    return m_sort_model->get_path(iter);
}

Gtk::TreeModel::iterator ChannelListTree::AddFolder(const UserSettingsGuildFoldersEntry &folder) {
    if (!folder.ID.has_value()) {
        // just a guild
        if (!folder.GuildIDs.empty()) {
            const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(folder.GuildIDs[0]);
            if (guild.has_value()) {
                return AddGuild(*guild, m_model->children());
            }
        }
    } else {
        auto folder_row = *m_model->append();
        folder_row[m_columns.m_type] = RenderType::Folder;
        folder_row[m_columns.m_id] = *folder.ID;
        m_tmp_row_map[*folder.ID] = folder_row;
        if (folder.Name.has_value()) {
            folder_row[m_columns.m_name] = Glib::Markup::escape_text(*folder.Name);
        } else {
            folder_row[m_columns.m_name] = "Folder";
        }
        if (folder.Color.has_value()) {
            folder_row[m_columns.m_color] = IntToRGBA(*folder.Color);
        }

        int sort_value = 0;
        for (const auto &guild_id : folder.GuildIDs) {
            const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(guild_id);
            if (guild.has_value()) {
                auto guild_row = AddGuild(*guild, folder_row->children());
                (*guild_row)[m_columns.m_sort] = sort_value++;
            }
        }

        return folder_row;
    }

    return {};
}

Gtk::TreeModel::iterator ChannelListTree::AddGuild(const GuildData &guild, const Gtk::TreeNodeChildren &root) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto &img = Abaddon::Get().GetImageManager();

    auto guild_row = *m_model->append(root);
    guild_row[m_columns.m_type] = RenderType::Guild;
    guild_row[m_columns.m_id] = guild.ID;
    guild_row[m_columns.m_name] = "<b>" + Glib::Markup::escape_text(guild.Name) + "</b>";
    guild_row[m_columns.m_icon] = img.GetPlaceholder(GuildIconSize);
    m_tmp_guild_row_map[guild.ID] = guild_row;

    if (Abaddon::Get().GetSettings().ShowAnimations && guild.HasAnimatedIcon()) {
        const auto cb = [this, id = guild.ID](const Glib::RefPtr<Gdk::PixbufAnimation> &pb) {
            auto iter = GetIteratorForGuildFromID(id);
            if (iter) (*iter)[m_columns.m_icon_anim] = pb;
        };
        img.LoadAnimationFromURL(guild.GetIconURL("gif", "32"), GuildIconSize, GuildIconSize, sigc::track_obj(cb, *this));
    } else if (guild.HasIcon()) {
        const auto cb = [this, id = guild.ID](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            auto iter = GetIteratorForGuildFromID(id);
            if (iter) (*iter)[m_columns.m_icon] = pb->scale_simple(GuildIconSize, GuildIconSize, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL(guild.GetIconURL("png", "32"), sigc::track_obj(cb, *this));
    }

    if (!guild.Channels.has_value()) return guild_row;

    // separate out the channels
    std::vector<ChannelData> orphan_channels;
    std::map<Snowflake, std::vector<ChannelData>> categories;

    for (const auto &channel_ : *guild.Channels) {
        const auto channel = discord.GetChannel(channel_.ID);
        if (!channel.has_value()) continue;
#ifdef WITH_VOICE
        if (channel->Type == ChannelType::GUILD_TEXT || channel->Type == ChannelType::GUILD_NEWS || channel->Type == ChannelType::GUILD_VOICE) {
#else
        if (channel->Type == ChannelType::GUILD_TEXT || channel->Type == ChannelType::GUILD_NEWS) {
#endif
            if (channel->ParentID.has_value())
                categories[*channel->ParentID].push_back(*channel);
            else
                orphan_channels.push_back(*channel);
        } else if (channel->Type == ChannelType::GUILD_CATEGORY) {
            categories[channel->ID];
        }
    }

    std::map<Snowflake, std::vector<ChannelData>> threads;
    for (const auto &tmp : *guild.Threads) {
        const auto thread = discord.GetChannel(tmp.ID);
        if (thread.has_value())
            threads[*thread->ParentID].push_back(*thread);
    }
    const auto add_threads = [&](const ChannelData &channel, const Gtk::TreeRow &row) {
        row[m_columns.m_expanded] = true;

        const auto it = threads.find(channel.ID);
        if (it == threads.end()) return;

        for (const auto &thread : it->second) {
            CreateThreadRow(row.children(), thread);
        }
    };

#ifdef WITH_VOICE
    auto add_voice_participants = [this, &discord](const ChannelData &channel, const Gtk::TreeNodeChildren &root) {
        for (auto user_id : discord.GetUsersInVoiceChannel(channel.ID)) {
            if (const auto user = discord.GetUser(user_id); user.has_value()) {
                CreateVoiceParticipantRow(*user, root);
            }
        }
    };
#endif

    for (const auto &channel : orphan_channels) {
        auto channel_row = *m_model->append(guild_row.children());
        if (IsTextChannel(channel.Type)) {
            channel_row[m_columns.m_type] = RenderType::TextChannel;
            channel_row[m_columns.m_name] = "#" + Glib::Markup::escape_text(*channel.Name);
        }
#ifdef WITH_VOICE
        else {
            channel_row[m_columns.m_type] = RenderType::VoiceChannel;
            channel_row[m_columns.m_name] = Glib::Markup::escape_text(*channel.Name);
            add_voice_participants(channel, channel_row->children());
        }
#endif
        channel_row[m_columns.m_id] = channel.ID;
        channel_row[m_columns.m_sort] = *channel.Position + OrphanChannelSortOffset;
        channel_row[m_columns.m_nsfw] = channel.NSFW();
        add_threads(channel, channel_row);
        m_tmp_row_map[channel.ID] = channel_row;
    }

    for (const auto &[category_id, channels] : categories) {
        const auto category = discord.GetChannel(category_id);
        if (!category.has_value()) continue;
        auto cat_row = *m_model->append(guild_row.children());
        cat_row[m_columns.m_type] = RenderType::Category;
        cat_row[m_columns.m_id] = category_id;
        cat_row[m_columns.m_name] = Glib::Markup::escape_text(*category->Name);
        cat_row[m_columns.m_sort] = *category->Position;
        cat_row[m_columns.m_expanded] = true;
        m_tmp_row_map[category_id] = cat_row;
        // m_view.expand_row wont work because it might not have channels

        for (const auto &channel : channels) {
            auto channel_row = *m_model->append(cat_row.children());
            if (IsTextChannel(channel.Type)) {
                channel_row[m_columns.m_type] = RenderType::TextChannel;
                channel_row[m_columns.m_name] = "#" + Glib::Markup::escape_text(*channel.Name);
            }
#ifdef WITH_VOICE
            else {
                channel_row[m_columns.m_type] = RenderType::VoiceChannel;
                channel_row[m_columns.m_name] = Glib::Markup::escape_text(*channel.Name);
                add_voice_participants(channel, channel_row->children());
            }
#endif
            channel_row[m_columns.m_id] = channel.ID;
            channel_row[m_columns.m_sort] = *channel.Position;
            channel_row[m_columns.m_nsfw] = channel.NSFW();
            add_threads(channel, channel_row);
            m_tmp_row_map[channel.ID] = channel_row;
        }
    }

    return guild_row;
}

Gtk::TreeModel::iterator ChannelListTree::UpdateCreateChannelCategory(const ChannelData &channel) {
    const auto iter = GetIteratorForGuildFromID(*channel.GuildID);
    if (!iter) return {};

    auto cat_row = *m_model->append(iter->children());
    cat_row[m_columns.m_type] = RenderType::Category;
    cat_row[m_columns.m_id] = channel.ID;
    cat_row[m_columns.m_name] = Glib::Markup::escape_text(*channel.Name);
    cat_row[m_columns.m_sort] = *channel.Position;
    cat_row[m_columns.m_expanded] = true;

    return cat_row;
}

Gtk::TreeModel::iterator ChannelListTree::CreateThreadRow(const Gtk::TreeNodeChildren &children, const ChannelData &channel) {
    auto thread_iter = m_model->append(children);
    auto thread_row = *thread_iter;
    thread_row[m_columns.m_type] = RenderType::Thread;
    thread_row[m_columns.m_id] = channel.ID;
    thread_row[m_columns.m_name] = "- " + Glib::Markup::escape_text(*channel.Name);
    thread_row[m_columns.m_sort] = static_cast<int64_t>(channel.ID);
    thread_row[m_columns.m_nsfw] = false;

    return thread_iter;
}

#ifdef WITH_VOICE
Gtk::TreeModel::iterator ChannelListTree::CreateVoiceParticipantRow(const UserData &user, const Gtk::TreeNodeChildren &parent) {
    auto row = *m_model->append(parent);
    row[m_columns.m_type] = RenderType::VoiceParticipant;
    row[m_columns.m_id] = user.ID;
    row[m_columns.m_name] = user.GetDisplayNameEscaped();

    const auto voice_state = Abaddon::Get().GetDiscordClient().GetVoiceState(user.ID);
    if (voice_state.has_value()) {
        row[m_columns.m_voice_flags] = voice_state->second;
    }

    auto &img = Abaddon::Get().GetImageManager();
    row[m_columns.m_icon] = img.GetPlaceholder(VoiceParticipantIconSize);
    const auto cb = [this, user_id = user.ID](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
        auto iter = GetIteratorForRowFromIDOfType(user_id, RenderType::VoiceParticipant);
        if (iter) (*iter)[m_columns.m_icon] = pb->scale_simple(VoiceParticipantIconSize, VoiceParticipantIconSize, Gdk::INTERP_BILINEAR);
    };
    img.LoadFromURL(user.GetAvatarURL("png", "32"), sigc::track_obj(cb, *this));

    return row;
}
#endif

void ChannelListTree::UpdateChannelCategory(const ChannelData &channel) {
    auto iter = GetIteratorForRowFromID(channel.ID);
    if (!iter) return;

    (*iter)[m_columns.m_sort] = *channel.Position;
    (*iter)[m_columns.m_name] = Glib::Markup::escape_text(*channel.Name);
}

// todo this all needs refactoring for shooore
Gtk::TreeModel::iterator ChannelListTree::GetIteratorForTopLevelFromID(Snowflake id) {
    for (const auto &child : m_model->children()) {
        if ((child[m_columns.m_type] == RenderType::Guild || child[m_columns.m_type] == RenderType::Folder) && child[m_columns.m_id] == id) {
            return child;
        } else if (child[m_columns.m_type] == RenderType::Folder) {
            for (const auto &folder_child : child->children()) {
                if (folder_child[m_columns.m_id] == id) {
                    return folder_child;
                }
            }
        }
    }
    return {};
}

Gtk::TreeModel::iterator ChannelListTree::GetIteratorForGuildFromID(Snowflake id) {
    for (const auto &child : m_model->children()) {
        if (child[m_columns.m_type] == RenderType::Guild && child[m_columns.m_id] == id) {
            return child;
        } else if (child[m_columns.m_type] == RenderType::Folder) {
            for (const auto &folder_child : child->children()) {
                if (folder_child[m_columns.m_id] == id) {
                    return folder_child;
                }
            }
        }
    }
    return {};
}

Gtk::TreeModel::iterator ChannelListTree::GetIteratorForRowFromID(Snowflake id) {
    std::queue<Gtk::TreeModel::iterator> queue;
    for (const auto &child : m_model->children())
        for (const auto &child2 : child.children())
            queue.push(child2);

    while (!queue.empty()) {
        auto item = queue.front();
        if ((*item)[m_columns.m_id] == id && (*item)[m_columns.m_type] != RenderType::Guild) return item;
        for (const auto &child : item->children())
            queue.push(child);
        queue.pop();
    }

    return {};
}

Gtk::TreeModel::iterator ChannelListTree::GetIteratorForRowFromIDOfType(Snowflake id, RenderType type) {
    std::queue<Gtk::TreeModel::iterator> queue;
    for (const auto &child : m_model->children())
        for (const auto &child2 : child.children())
            queue.push(child2);

    while (!queue.empty()) {
        auto item = queue.front();
        if ((*item)[m_columns.m_type] == type && (*item)[m_columns.m_id] == id) return item;
        for (const auto &child : item->children())
            queue.push(child);
        queue.pop();
    }

    return {};
}

bool ChannelListTree::IsTextChannel(ChannelType type) {
    return type == ChannelType::GUILD_TEXT || type == ChannelType::GUILD_NEWS;
}

// this should be unncessary but something is behaving strange so its just in case
void ChannelListTree::OnRowCollapsed(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path) const {
    (*iter)[m_columns.m_expanded] = false;
}

void ChannelListTree::OnRowExpanded(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path) {
    // restore previous expansion
    auto model_iter = ConvertViewIterToModel(iter);
    for (auto it = model_iter->children().begin(); it != model_iter->children().end(); it++) {
        if ((*it)[m_columns.m_expanded]) {
            m_view.expand_row(GetViewPathFromViewIter(ConvertModelIterToView(it)), false);
        }
    }

    // try and restore selection if previous collapsed
    if (auto selection = m_view.get_selection(); selection && !selection->get_selected()) {
        selection->select(m_last_selected);
    }

    (*model_iter)[m_columns.m_expanded] = true;
}

bool ChannelListTree::SelectionFunc(const Glib::RefPtr<Gtk::TreeModel> &model, const Gtk::TreeModel::Path &path, bool is_currently_selected) {
    if (auto selection = m_view.get_selection()) {
        if (auto row = selection->get_selected()) {
            m_last_selected = GetViewPathFromViewIter(row);
        }
    }

    auto type = (*model->get_iter(path))[m_columns.m_type];
    return type == RenderType::TextChannel || type == RenderType::DM || type == RenderType::Thread;
}

void ChannelListTree::AddPrivateChannels() {
    auto header_row = *m_model->append();
    header_row[m_columns.m_type] = RenderType::DMHeader;
    header_row[m_columns.m_sort] = -1;
    header_row[m_columns.m_name] = "<b>Direct Messages</b>";
    m_dm_header = m_model->get_path(header_row);

    auto &discord = Abaddon::Get().GetDiscordClient();
    auto &img = Abaddon::Get().GetImageManager();

    const auto dm_ids = discord.GetPrivateChannels();
    for (const auto dm_id : dm_ids) {
        const auto dm = discord.GetChannel(dm_id);
        if (!dm.has_value()) continue;

        std::optional<UserData> top_recipient;
        const auto recipients = dm->GetDMRecipients();
        if (!recipients.empty())
            top_recipient = recipients[0];

        auto iter = m_model->append(header_row->children());
        auto row = *iter;
        row[m_columns.m_type] = RenderType::DM;
        row[m_columns.m_id] = dm_id;
        row[m_columns.m_name] = Glib::Markup::escape_text(dm->GetDisplayName());
        row[m_columns.m_sort] = static_cast<int64_t>(-(dm->LastMessageID.has_value() ? *dm->LastMessageID : dm_id));
        row[m_columns.m_icon] = img.GetPlaceholder(DMIconSize);
        row[m_columns.m_expanded] = true;

#ifdef WITH_VOICE
        for (auto user_id : discord.GetUsersInVoiceChannel(dm_id)) {
            if (const auto user = discord.GetUser(user_id); user.has_value()) {
                CreateVoiceParticipantRow(*user, row->children());
            }
        }
#endif

        SetDMChannelIcon(iter, *dm);
    }
}

void ChannelListTree::UpdateCreateDMChannel(const ChannelData &dm) {
    auto header_row = m_model->get_iter(m_dm_header);
    auto &img = Abaddon::Get().GetImageManager();

    auto iter = m_model->append(header_row->children());
    auto row = *iter;
    row[m_columns.m_type] = RenderType::DM;
    row[m_columns.m_id] = dm.ID;
    row[m_columns.m_name] = Glib::Markup::escape_text(dm.GetDisplayName());
    row[m_columns.m_sort] = static_cast<int64_t>(-(dm.LastMessageID.has_value() ? *dm.LastMessageID : dm.ID));
    row[m_columns.m_icon] = img.GetPlaceholder(DMIconSize);

    SetDMChannelIcon(iter, dm);
}

void ChannelListTree::SetDMChannelIcon(Gtk::TreeIter iter, const ChannelData &dm) {
    auto &img = Abaddon::Get().GetImageManager();

    std::optional<UserData> top_recipient;
    const auto recipients = dm.GetDMRecipients();
    if (!recipients.empty())
        top_recipient = recipients[0];

    if (dm.HasIcon()) {
        const auto cb = [this, iter](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            if (iter)
                (*iter)[m_columns.m_icon] = pb->scale_simple(DMIconSize, DMIconSize, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL(dm.GetIconURL(), sigc::track_obj(cb, *this));
    } else if (dm.Type == ChannelType::DM && top_recipient.has_value()) {
        const auto cb = [this, iter](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            if (iter)
                (*iter)[m_columns.m_icon] = pb->scale_simple(DMIconSize, DMIconSize, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL(top_recipient->GetAvatarURL("png", "32"), sigc::track_obj(cb, *this));
    } else { // GROUP_DM
        std::string hash;
        switch (dm.ID.GetUnixMilliseconds() % 8) {
            case 0:
                hash = "ee9275c5a437f7dc7f9430ba95f12ebd";
                break;
            case 1:
                hash = "9baf45aac2a0ec2e2dab288333acb9d9";
                break;
            case 2:
                hash = "7ba11ffb1900fa2b088cb31324242047";
                break;
            case 3:
                hash = "f90fca70610c4898bc57b58bce92f587";
                break;
            case 4:
                hash = "e2779af34b8d9126b77420e5f09213ce";
                break;
            case 5:
                hash = "c6851bd0b03f1cca5a8c1e720ea6ea17";
                break;
            case 6:
                hash = "f7e38ac976a2a696161c923502a8345b";
                break;
            case 7:
            default:
                hash = "3cb840d03313467838d658bbec801fcd";
                break;
        }
        const auto cb = [this, iter](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            if (iter)
                (*iter)[m_columns.m_icon] = pb->scale_simple(DMIconSize, DMIconSize, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL("https://discord.com/assets/" + hash + ".png", sigc::track_obj(cb, *this));
    }
}

void ChannelListTree::RedrawUnreadIndicatorsForChannel(const ChannelData &channel) {
    if (channel.GuildID.has_value()) {
        auto iter = GetIteratorForGuildFromID(*channel.GuildID);
        if (iter) m_model->row_changed(m_model->get_path(iter), iter);
    }
    if (channel.ParentID.has_value()) {
        auto iter = GetIteratorForRowFromIDOfType(*channel.ParentID, RenderType::Category);
        if (iter) m_model->row_changed(m_model->get_path(iter), iter);
    }
}

void ChannelListTree::OnMessageAck(const MessageAckData &data) {
    // trick renderer into redrawing
    m_model->row_changed(Gtk::TreeModel::Path("0"), m_model->get_iter("0")); // 0 is always path for dm header
    auto iter = GetIteratorForRowFromID(data.ChannelID);
    if (iter) m_model->row_changed(m_model->get_path(iter), iter);
    auto channel = Abaddon::Get().GetDiscordClient().GetChannel(data.ChannelID);
    if (channel.has_value()) {
        RedrawUnreadIndicatorsForChannel(*channel);
    }
}

void ChannelListTree::OnMessageCreate(const Message &msg) {
    auto iter = GetIteratorForRowFromID(msg.ChannelID);
    if (iter) m_model->row_changed(m_model->get_path(iter), iter); // redraw
    const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(msg.ChannelID);
    if (!channel.has_value()) return;
    if (channel->Type == ChannelType::DM || channel->Type == ChannelType::GROUP_DM) {
        if (iter)
            (*iter)[m_columns.m_sort] = static_cast<int64_t>(-msg.ID);
    }
    RedrawUnreadIndicatorsForChannel(*channel);
}

bool ChannelListTree::OnButtonPressEvent(GdkEventButton *ev) {
    if (ev->button == GDK_BUTTON_SECONDARY && ev->type == GDK_BUTTON_PRESS) {
        if (m_view.get_path_at_pos(static_cast<int>(ev->x), static_cast<int>(ev->y), m_path_for_menu)) {
            m_path_for_menu = m_filter_model->convert_path_to_child_path(m_sort_model->convert_path_to_child_path(m_path_for_menu));
            if (!m_path_for_menu) return true;
            auto row = (*m_model->get_iter(m_path_for_menu));
            switch (static_cast<RenderType>(row[m_columns.m_type])) {
                case RenderType::Guild:
                    OnGuildSubmenuPopup();
                    m_menu_guild.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                    break;
                case RenderType::Category:
                    OnCategorySubmenuPopup();
                    m_menu_category.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                    break;
                case RenderType::TextChannel:
                    OnChannelSubmenuPopup();
                    m_menu_channel.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                    break;
#ifdef WITH_VOICE
                case RenderType::VoiceChannel:
                    OnVoiceChannelSubmenuPopup();
                    m_menu_voice_channel.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                    break;
#endif
                case RenderType::DM: {
                    OnDMSubmenuPopup();
                    const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(static_cast<Snowflake>(row[m_columns.m_id]));
                    if (channel.has_value()) {
                        m_menu_dm_close.set_label(channel->Type == ChannelType::DM ? "Close" : "Leave");
                        m_menu_dm_close.show();
                    } else
                        m_menu_dm_close.hide();
                    m_menu_dm.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                } break;
                case RenderType::Thread: {
                    OnThreadSubmenuPopup();
                    m_menu_thread.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                    break;
                } break;
                default:
                    break;
            }
        }
        return true;
    }
    return false;
}

void ChannelListTree::MoveRow(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::iterator &new_parent) {
    // duplicate the row data under the new parent and then delete the old row
    auto row = *m_model->append(new_parent->children());
    // would be nice to be able to get all columns out at runtime so i dont need this
#define M(name) \
    row[m_columns.name] = static_cast<decltype(m_columns.name)::ElementType>((*iter)[m_columns.name]);
    M(m_type);
    M(m_id);
    M(m_name);
    M(m_icon);
    M(m_icon_anim);
    M(m_sort);
    M(m_nsfw);
    M(m_expanded);
    M(m_color);
#undef M

    // recursively move children
    // weird construct to work around iterator invalidation (at least i think thats what the problem was)
    const auto tmp = iter->children();
    const auto children = std::vector<Gtk::TreeRow>(tmp.begin(), tmp.end());
    for (size_t i = 0; i < children.size(); i++)
        MoveRow(children[i], row);

    // delete original
    m_model->erase(iter);
}

void ChannelListTree::OnGuildSubmenuPopup() {
    const auto iter = m_model->get_iter(m_path_for_menu);
    if (!iter) return;
    const auto id = static_cast<Snowflake>((*iter)[m_columns.m_id]);
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (discord.IsGuildMuted(id))
        m_menu_guild_toggle_mute.set_label("Unmute");
    else
        m_menu_guild_toggle_mute.set_label("Mute");

    const auto guild = discord.GetGuild(id);
    const auto self_id = discord.GetUserData().ID;
    m_menu_guild_leave.set_sensitive(!(guild.has_value() && guild->OwnerID == self_id));
}

void ChannelListTree::OnCategorySubmenuPopup() {
    const auto iter = m_model->get_iter(m_path_for_menu);
    if (!iter) return;
    const auto id = static_cast<Snowflake>((*iter)[m_columns.m_id]);
    if (Abaddon::Get().GetDiscordClient().IsChannelMuted(id))
        m_menu_category_toggle_mute.set_label("Unmute");
    else
        m_menu_category_toggle_mute.set_label("Mute");
}

void ChannelListTree::OnChannelSubmenuPopup() {
    const auto iter = m_model->get_iter(m_path_for_menu);
    if (!iter) return;
    const auto id = static_cast<Snowflake>((*iter)[m_columns.m_id]);
    auto &discord = Abaddon::Get().GetDiscordClient();
#ifdef WITH_LIBHANDY
    const auto perms = discord.HasChannelPermission(discord.GetUserData().ID, id, Permission::VIEW_CHANNEL);
    m_menu_channel_open_tab.set_sensitive(perms);
#endif
    if (discord.IsChannelMuted(id))
        m_menu_channel_toggle_mute.set_label("Unmute");
    else
        m_menu_channel_toggle_mute.set_label("Mute");
}

#ifdef WITH_VOICE
void ChannelListTree::OnVoiceChannelSubmenuPopup() {
    const auto iter = m_model->get_iter(m_path_for_menu);
    if (!iter) return;
    const auto id = static_cast<Snowflake>((*iter)[m_columns.m_id]);
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (discord.IsVoiceConnected() || discord.IsVoiceConnecting()) {
        m_menu_voice_channel_join.set_sensitive(false);
        m_menu_voice_channel_disconnect.set_sensitive(discord.GetVoiceChannelID() == id);
    } else {
        m_menu_voice_channel_join.set_sensitive(true);
        m_menu_voice_channel_disconnect.set_sensitive(false);
    }
}
#endif

void ChannelListTree::OnDMSubmenuPopup() {
    auto iter = m_model->get_iter(m_path_for_menu);
    if (!iter) return;
    const auto id = static_cast<Snowflake>((*iter)[m_columns.m_id]);
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (discord.IsChannelMuted(id))
        m_menu_dm_toggle_mute.set_label("Unmute");
    else
        m_menu_dm_toggle_mute.set_label("Mute");

#ifdef WITH_VOICE
    if (discord.IsVoiceConnected() || discord.IsVoiceConnecting()) {
        m_menu_dm_join_voice.set_sensitive(false);
        m_menu_dm_disconnect_voice.set_sensitive(discord.GetVoiceChannelID() == id);
    } else {
        m_menu_dm_join_voice.set_sensitive(true);
        m_menu_dm_disconnect_voice.set_sensitive(false);
    }
#endif
}

void ChannelListTree::OnThreadSubmenuPopup() {
    m_menu_thread_archive.set_visible(false);
    m_menu_thread_unarchive.set_visible(false);

    auto &discord = Abaddon::Get().GetDiscordClient();
    auto iter = m_model->get_iter(m_path_for_menu);
    if (!iter) return;
    const auto id = static_cast<Snowflake>((*iter)[m_columns.m_id]);

    if (discord.IsChannelMuted(id))
        m_menu_thread_toggle_mute.set_label("Unmute");
    else
        m_menu_thread_toggle_mute.set_label("Mute");

    auto channel = discord.GetChannel(id);
    if (!channel.has_value() || !channel->ThreadMetadata.has_value()) return;
    if (!discord.HasGuildPermission(discord.GetUserData().ID, *channel->GuildID, Permission::MANAGE_THREADS)) return;

    m_menu_thread_archive.set_visible(!channel->ThreadMetadata->IsArchived);
    m_menu_thread_unarchive.set_visible(channel->ThreadMetadata->IsArchived);
}

ChannelListTree::type_signal_action_channel_item_select ChannelListTree::signal_action_channel_item_select() {
    return m_signal_action_channel_item_select;
}

ChannelListTree::type_signal_action_guild_leave ChannelListTree::signal_action_guild_leave() {
    return m_signal_action_guild_leave;
}

ChannelListTree::type_signal_action_guild_settings ChannelListTree::signal_action_guild_settings() {
    return m_signal_action_guild_settings;
}

#ifdef WITH_LIBHANDY
ChannelListTree::type_signal_action_open_new_tab ChannelListTree::signal_action_open_new_tab() {
    return m_signal_action_open_new_tab;
}
#endif

#ifdef WITH_VOICE
ChannelListTree::type_signal_action_join_voice_channel ChannelListTree::signal_action_join_voice_channel() {
    return m_signal_action_join_voice_channel;
}

ChannelListTree::type_signal_action_disconnect_voice ChannelListTree::signal_action_disconnect_voice() {
    return m_signal_action_disconnect_voice;
}
#endif

ChannelListTree::ModelColumns::ModelColumns() {
    add(m_type);
    add(m_id);
    add(m_name);
    add(m_icon);
    add(m_icon_anim);
    add(m_sort);
    add(m_nsfw);
    add(m_expanded);
    add(m_color);
    add(m_voice_flags);
}
