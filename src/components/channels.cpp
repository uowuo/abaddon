#include "channels.hpp"
#include "imgmanager.hpp"
#include "statusindicator.hpp"
#include <algorithm>
#include <map>
#include <unordered_map>

ChannelList::ChannelList()
    : Glib::ObjectBase(typeid(ChannelList))
    , m_model(Gtk::TreeStore::create(m_columns))
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
    , m_menu_dm_copy_id("_Copy ID", true)
    , m_menu_dm_close("") // changes depending on if group or not
    , m_menu_thread_copy_id("_Copy ID", true)
    , m_menu_thread_leave("_Leave", true)
    , m_menu_thread_archive("_Archive", true)
    , m_menu_thread_unarchive("_Unarchive", true)
    , m_menu_thread_mark_as_read("Mark as _Read", true) {
    get_style_context()->add_class("channel-list");

    // todo: move to method
    const auto cb = [this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column) {
        auto row = *m_model->get_iter(path);
        const auto type = row[m_columns.m_type];
        // text channels should not be allowed to be collapsed
        // maybe they should be but it seems a little difficult to handle expansion to permit this
        if (type != RenderType::TextChannel) {
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
    m_view.signal_row_collapsed().connect(sigc::mem_fun(*this, &ChannelList::OnRowCollapsed), false);
    m_view.signal_row_expanded().connect(sigc::mem_fun(*this, &ChannelList::OnRowExpanded), false);
    m_view.set_activate_on_single_click(true);
    m_view.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
    m_view.get_selection()->set_select_function(sigc::mem_fun(*this, &ChannelList::SelectionFunc));
    m_view.signal_button_press_event().connect(sigc::mem_fun(*this, &ChannelList::OnButtonPressEvent), false);

    m_view.set_hexpand(true);
    m_view.set_vexpand(true);

    m_view.set_show_expanders(false);
    m_view.set_enable_search(false);
    m_view.set_headers_visible(false);
    m_view.set_model(m_model);
    m_model->set_sort_column(m_columns.m_sort, Gtk::SORT_ASCENDING);

    m_model->signal_row_inserted().connect([this](const Gtk::TreeModel::Path &path, const Gtk::TreeModel::iterator &iter) {
        if (m_updating_listing) return;
        if (auto parent = iter->parent(); parent && (*parent)[m_columns.m_expanded])
            m_view.expand_row(m_model->get_path(parent), false);
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
    discord.signal_message_create().connect(sigc::mem_fun(*this, &ChannelList::OnMessageCreate));
    discord.signal_guild_create().connect(sigc::mem_fun(*this, &ChannelList::UpdateNewGuild));
    discord.signal_guild_delete().connect(sigc::mem_fun(*this, &ChannelList::UpdateRemoveGuild));
    discord.signal_channel_delete().connect(sigc::mem_fun(*this, &ChannelList::UpdateRemoveChannel));
    discord.signal_channel_update().connect(sigc::mem_fun(*this, &ChannelList::UpdateChannel));
    discord.signal_channel_create().connect(sigc::mem_fun(*this, &ChannelList::UpdateCreateChannel));
    discord.signal_thread_delete().connect(sigc::mem_fun(*this, &ChannelList::OnThreadDelete));
    discord.signal_thread_update().connect(sigc::mem_fun(*this, &ChannelList::OnThreadUpdate));
    discord.signal_thread_list_sync().connect(sigc::mem_fun(*this, &ChannelList::OnThreadListSync));
    discord.signal_added_to_thread().connect(sigc::mem_fun(*this, &ChannelList::OnThreadJoined));
    discord.signal_removed_from_thread().connect(sigc::mem_fun(*this, &ChannelList::OnThreadRemoved));
    discord.signal_guild_update().connect(sigc::mem_fun(*this, &ChannelList::UpdateGuild));
    discord.signal_message_ack().connect(sigc::mem_fun(*this, &ChannelList::OnMessageAck));
    discord.signal_channel_muted().connect(sigc::mem_fun(*this, &ChannelList::OnChannelMute));
    discord.signal_channel_unmuted().connect(sigc::mem_fun(*this, &ChannelList::OnChannelUnmute));
    discord.signal_guild_muted().connect(sigc::mem_fun(*this, &ChannelList::OnGuildMute));
    discord.signal_guild_unmuted().connect(sigc::mem_fun(*this, &ChannelList::OnGuildUnmute));
}

void ChannelList::UsePanedHack(Gtk::Paned &paned) {
    paned.property_position().signal_changed().connect(sigc::mem_fun(*this, &ChannelList::OnPanedPositionChanged));
}

void ChannelList::OnPanedPositionChanged() {
    m_view.queue_draw();
}

void ChannelList::UpdateListing() {
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
    if (folders.empty()) {
        // fallback if no organization has occurred (guild_folders will be empty)
        const auto guild_ids = discord.GetUserSortedGuilds();
        for (const auto &guild_id : guild_ids) {
            const auto guild = discord.GetGuild(guild_id);
            if (!guild.has_value()) continue;

            auto iter = AddGuild(*guild, m_model->children());
            if (iter) (*iter)[m_columns.m_sort] = sort_value++;
        }
    } else {
        for (const auto &group : folders) {
            auto iter = AddFolder(group);
            if (iter) (*iter)[m_columns.m_sort] = sort_value++;
        }
    }

    m_updating_listing = false;

    AddPrivateChannels();
}

// TODO update for folders
void ChannelList::UpdateNewGuild(const GuildData &guild) {
    AddGuild(guild, m_model->children());
    // update sort order
    int sortnum = 0;
    for (const auto guild_id : Abaddon::Get().GetDiscordClient().GetUserSortedGuilds()) {
        auto iter = GetIteratorForGuildFromID(guild_id);
        if (iter)
            (*iter)[m_columns.m_sort] = ++sortnum;
    }
}

void ChannelList::UpdateRemoveGuild(Snowflake id) {
    auto iter = GetIteratorForGuildFromID(id);
    if (!iter) return;
    m_model->erase(iter);
}

void ChannelList::UpdateRemoveChannel(Snowflake id) {
    auto iter = GetIteratorForChannelFromID(id);
    if (!iter) return;
    m_model->erase(iter);
}

void ChannelList::UpdateChannel(Snowflake id) {
    auto iter = GetIteratorForChannelFromID(id);
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
        new_parent = GetIteratorForChannelFromID(*channel->ParentID);
    else if (channel->GuildID.has_value())
        new_parent = GetIteratorForGuildFromID(*channel->GuildID);

    if (new_parent && iter->parent() != new_parent)
        MoveRow(iter, new_parent);
}

void ChannelList::UpdateCreateChannel(const ChannelData &channel) {
    if (channel.Type == ChannelType::GUILD_CATEGORY) return (void)UpdateCreateChannelCategory(channel);
    if (channel.Type == ChannelType::DM || channel.Type == ChannelType::GROUP_DM) return UpdateCreateDMChannel(channel);
    if (channel.Type != ChannelType::GUILD_TEXT && channel.Type != ChannelType::GUILD_NEWS) return;

    Gtk::TreeRow channel_row;
    bool orphan;
    if (channel.ParentID.has_value()) {
        orphan = false;
        auto iter = GetIteratorForChannelFromID(*channel.ParentID);
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

void ChannelList::UpdateGuild(Snowflake id) {
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

void ChannelList::OnThreadJoined(Snowflake id) {
    if (GetIteratorForChannelFromID(id)) return;
    const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(id);
    if (!channel.has_value()) return;
    const auto parent = GetIteratorForChannelFromID(*channel->ParentID);
    if (parent)
        CreateThreadRow(parent->children(), *channel);
}

void ChannelList::OnThreadRemoved(Snowflake id) {
    DeleteThreadRow(id);
}

void ChannelList::OnThreadDelete(const ThreadDeleteData &data) {
    DeleteThreadRow(data.ID);
}

// todo probably make the row stick around if its selected until the selection changes
void ChannelList::OnThreadUpdate(const ThreadUpdateData &data) {
    auto iter = GetIteratorForChannelFromID(data.Thread.ID);
    if (iter)
        (*iter)[m_columns.m_name] = "- " + Glib::Markup::escape_text(*data.Thread.Name);

    if (data.Thread.ThreadMetadata->IsArchived)
        DeleteThreadRow(data.Thread.ID);
}

void ChannelList::OnThreadListSync(const ThreadListSyncData &data) {
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
            auto iter = GetIteratorForChannelFromID(thread_id);
            m_model->erase(iter);
        }
    }

    // delete all archived threads
    for (auto thread : data.Threads) {
        if (thread.ThreadMetadata->IsArchived) {
            if (auto iter = GetIteratorForChannelFromID(thread.ID))
                m_model->erase(iter);
        }
    }
}

void ChannelList::DeleteThreadRow(Snowflake id) {
    auto iter = GetIteratorForChannelFromID(id);
    if (iter)
        m_model->erase(iter);
}

void ChannelList::OnChannelMute(Snowflake id) {
    if (auto iter = GetIteratorForChannelFromID(id))
        m_model->row_changed(m_model->get_path(iter), iter);
}

void ChannelList::OnChannelUnmute(Snowflake id) {
    if (auto iter = GetIteratorForChannelFromID(id))
        m_model->row_changed(m_model->get_path(iter), iter);
}

void ChannelList::OnGuildMute(Snowflake id) {
    if (auto iter = GetIteratorForGuildFromID(id))
        m_model->row_changed(m_model->get_path(iter), iter);
}

void ChannelList::OnGuildUnmute(Snowflake id) {
    if (auto iter = GetIteratorForGuildFromID(id))
        m_model->row_changed(m_model->get_path(iter), iter);
}

// create a temporary channel row for non-joined threads
// and delete them when the active channel switches off of them if still not joined
void ChannelList::SetActiveChannel(Snowflake id, bool expand_to) {
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

    const auto channel_iter = GetIteratorForChannelFromID(id);
    if (channel_iter) {
        if (expand_to) {
            m_view.expand_to_path(m_model->get_path(channel_iter));
        }
        m_view.get_selection()->select(channel_iter);
    } else {
        m_view.get_selection()->unselect_all();
        const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(id);
        if (!channel.has_value() || !channel->IsThread()) return;
        auto parent_iter = GetIteratorForChannelFromID(*channel->ParentID);
        if (!parent_iter) return;
        m_temporary_thread_row = CreateThreadRow(parent_iter->children(), *channel);
        m_view.get_selection()->select(m_temporary_thread_row);
    }
}

void ChannelList::UseExpansionState(const ExpansionStateRoot &root) {
    auto recurse = [this](auto &self, const ExpansionStateRoot &root) -> void {
        for (const auto &[id, state] : root.Children) {
            Gtk::TreeModel::iterator row_iter;
            if (const auto map_iter = m_tmp_row_map.find(id); map_iter != m_tmp_row_map.end()) {
                row_iter = map_iter->second;
            } else if (const auto map_iter = m_tmp_guild_row_map.find(id); map_iter != m_tmp_guild_row_map.end()) {
                row_iter = map_iter->second;
            }

            if (row_iter) {
                if (state.IsExpanded)
                    m_view.expand_row(m_model->get_path(row_iter), false);
                else
                    m_view.collapse_row(m_model->get_path(row_iter));
            }

            self(self, state.Children);
        }
    };

    for (const auto &[id, state] : root.Children) {
        if (const auto iter = GetIteratorForTopLevelFromID(id)) {
            if (state.IsExpanded)
                m_view.expand_row(m_model->get_path(iter), false);
            else
                m_view.collapse_row(m_model->get_path(iter));
        }

        recurse(recurse, state.Children);
    }

    m_tmp_row_map.clear();
}

ExpansionStateRoot ChannelList::GetExpansionState() const {
    ExpansionStateRoot r;

    auto recurse = [this](auto &self, const Gtk::TreeRow &row) -> ExpansionState {
        ExpansionState r;

        r.IsExpanded = row[m_columns.m_expanded];
        for (const auto &child : row.children())
            r.Children.Children[static_cast<Snowflake>(child[m_columns.m_id])] = self(self, child);

        return r;
    };

    for (const auto &child : m_model->children()) {
        const auto id = static_cast<Snowflake>(child[m_columns.m_id]);
        if (static_cast<uint64_t>(id) == 0ULL) continue; // dont save DM header
        r.Children[id] = recurse(recurse, child);
    }

    return r;
}

Gtk::TreeModel::iterator ChannelList::AddFolder(const UserSettingsGuildFoldersEntry &folder) {
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

Gtk::TreeModel::iterator ChannelList::AddGuild(const GuildData &guild, const Gtk::TreeNodeChildren &root) {
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
        if (channel->Type == ChannelType::GUILD_TEXT || channel->Type == ChannelType::GUILD_NEWS) {
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

        for (const auto &thread : it->second)
            m_tmp_row_map[thread.ID] = CreateThreadRow(row.children(), thread);
    };

    for (const auto &channel : orphan_channels) {
        auto channel_row = *m_model->append(guild_row.children());
        channel_row[m_columns.m_type] = RenderType::TextChannel;
        channel_row[m_columns.m_id] = channel.ID;
        channel_row[m_columns.m_name] = "#" + Glib::Markup::escape_text(*channel.Name);
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
            channel_row[m_columns.m_type] = RenderType::TextChannel;
            channel_row[m_columns.m_id] = channel.ID;
            channel_row[m_columns.m_name] = "#" + Glib::Markup::escape_text(*channel.Name);
            channel_row[m_columns.m_sort] = *channel.Position;
            channel_row[m_columns.m_nsfw] = channel.NSFW();
            add_threads(channel, channel_row);
            m_tmp_row_map[channel.ID] = channel_row;
        }
    }

    return guild_row;
}

Gtk::TreeModel::iterator ChannelList::UpdateCreateChannelCategory(const ChannelData &channel) {
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

Gtk::TreeModel::iterator ChannelList::CreateThreadRow(const Gtk::TreeNodeChildren &children, const ChannelData &channel) {
    auto thread_iter = m_model->append(children);
    auto thread_row = *thread_iter;
    thread_row[m_columns.m_type] = RenderType::Thread;
    thread_row[m_columns.m_id] = channel.ID;
    thread_row[m_columns.m_name] = "- " + Glib::Markup::escape_text(*channel.Name);
    thread_row[m_columns.m_sort] = static_cast<int64_t>(channel.ID);
    thread_row[m_columns.m_nsfw] = false;

    return thread_iter;
}

void ChannelList::UpdateChannelCategory(const ChannelData &channel) {
    auto iter = GetIteratorForChannelFromID(channel.ID);
    if (!iter) return;

    (*iter)[m_columns.m_sort] = *channel.Position;
    (*iter)[m_columns.m_name] = Glib::Markup::escape_text(*channel.Name);
}

// todo this all needs refactoring for shooore
Gtk::TreeModel::iterator ChannelList::GetIteratorForTopLevelFromID(Snowflake id) {
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

Gtk::TreeModel::iterator ChannelList::GetIteratorForGuildFromID(Snowflake id) {
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

Gtk::TreeModel::iterator ChannelList::GetIteratorForChannelFromID(Snowflake id) {
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

bool ChannelList::IsTextChannel(ChannelType type) {
    return type == ChannelType::GUILD_TEXT || type == ChannelType::GUILD_NEWS;
}

// this should be unncessary but something is behaving strange so its just in case
void ChannelList::OnRowCollapsed(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path) const {
    (*iter)[m_columns.m_expanded] = false;
}

void ChannelList::OnRowExpanded(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path) {
    // restore previous expansion
    for (auto it = iter->children().begin(); it != iter->children().end(); it++) {
        if ((*it)[m_columns.m_expanded])
            m_view.expand_row(m_model->get_path(it), false);
    }

    // try and restore selection if previous collapsed
    if (auto selection = m_view.get_selection(); selection && !selection->get_selected()) {
        selection->select(m_last_selected);
    }

    (*iter)[m_columns.m_expanded] = true;
}

bool ChannelList::SelectionFunc(const Glib::RefPtr<Gtk::TreeModel> &model, const Gtk::TreeModel::Path &path, bool is_currently_selected) {
    if (auto selection = m_view.get_selection())
        if (auto row = selection->get_selected())
            m_last_selected = m_model->get_path(row);

    auto type = (*m_model->get_iter(path))[m_columns.m_type];
    return type == RenderType::TextChannel || type == RenderType::DM || type == RenderType::Thread;
}

void ChannelList::AddPrivateChannels() {
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

        if (dm->HasIcon()) {
            const auto cb = [this, iter](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
                if (iter)
                    (*iter)[m_columns.m_icon] = pb->scale_simple(DMIconSize, DMIconSize, Gdk::INTERP_BILINEAR);
            };
            img.LoadFromURL(dm->GetIconURL(), sigc::track_obj(cb, *this));
        } else if (top_recipient.has_value()) {
            const auto cb = [this, iter](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
                if (iter)
                    (*iter)[m_columns.m_icon] = pb->scale_simple(DMIconSize, DMIconSize, Gdk::INTERP_BILINEAR);
            };
            img.LoadFromURL(top_recipient->GetAvatarURL("png", "32"), sigc::track_obj(cb, *this));
        }
    }
}

void ChannelList::UpdateCreateDMChannel(const ChannelData &dm) {
    auto header_row = m_model->get_iter(m_dm_header);
    auto &img = Abaddon::Get().GetImageManager();

    std::optional<UserData> top_recipient;
    const auto recipients = dm.GetDMRecipients();
    if (!recipients.empty())
        top_recipient = recipients[0];

    auto iter = m_model->append(header_row->children());
    auto row = *iter;
    row[m_columns.m_type] = RenderType::DM;
    row[m_columns.m_id] = dm.ID;
    row[m_columns.m_name] = Glib::Markup::escape_text(dm.GetDisplayName());
    row[m_columns.m_sort] = static_cast<int64_t>(-(dm.LastMessageID.has_value() ? *dm.LastMessageID : dm.ID));
    row[m_columns.m_icon] = img.GetPlaceholder(DMIconSize);

    if (top_recipient.has_value()) {
        const auto cb = [this, iter](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            if (iter)
                (*iter)[m_columns.m_icon] = pb->scale_simple(DMIconSize, DMIconSize, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL(top_recipient->GetAvatarURL("png", "32"), sigc::track_obj(cb, *this));
    }
}

void ChannelList::OnMessageAck(const MessageAckData &data) {
    // trick renderer into redrawing
    m_model->row_changed(Gtk::TreeModel::Path("0"), m_model->get_iter("0")); // 0 is always path for dm header
    auto iter = GetIteratorForChannelFromID(data.ChannelID);
    if (iter) m_model->row_changed(m_model->get_path(iter), iter);
    auto channel = Abaddon::Get().GetDiscordClient().GetChannel(data.ChannelID);
    if (channel.has_value() && channel->GuildID.has_value()) {
        iter = GetIteratorForGuildFromID(*channel->GuildID);
        if (iter) m_model->row_changed(m_model->get_path(iter), iter);
    }
}

void ChannelList::OnMessageCreate(const Message &msg) {
    auto iter = GetIteratorForChannelFromID(msg.ChannelID);
    if (iter) m_model->row_changed(m_model->get_path(iter), iter); // redraw
    const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(msg.ChannelID);
    if (!channel.has_value()) return;
    if (channel->Type == ChannelType::DM || channel->Type == ChannelType::GROUP_DM) {
        if (iter)
            (*iter)[m_columns.m_sort] = static_cast<int64_t>(-msg.ID);
    }
    if (channel->GuildID.has_value())
        if ((iter = GetIteratorForGuildFromID(*channel->GuildID)))
            m_model->row_changed(m_model->get_path(iter), iter);
}

bool ChannelList::OnButtonPressEvent(GdkEventButton *ev) {
    if (ev->button == GDK_BUTTON_SECONDARY && ev->type == GDK_BUTTON_PRESS) {
        if (m_view.get_path_at_pos(static_cast<int>(ev->x), static_cast<int>(ev->y), m_path_for_menu)) {
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

void ChannelList::MoveRow(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::iterator &new_parent) {
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

void ChannelList::OnGuildSubmenuPopup() {
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

void ChannelList::OnCategorySubmenuPopup() {
    const auto iter = m_model->get_iter(m_path_for_menu);
    if (!iter) return;
    const auto id = static_cast<Snowflake>((*iter)[m_columns.m_id]);
    if (Abaddon::Get().GetDiscordClient().IsChannelMuted(id))
        m_menu_category_toggle_mute.set_label("Unmute");
    else
        m_menu_category_toggle_mute.set_label("Mute");
}

void ChannelList::OnChannelSubmenuPopup() {
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

void ChannelList::OnDMSubmenuPopup() {
    auto iter = m_model->get_iter(m_path_for_menu);
    if (!iter) return;
    const auto id = static_cast<Snowflake>((*iter)[m_columns.m_id]);
    if (Abaddon::Get().GetDiscordClient().IsChannelMuted(id))
        m_menu_dm_toggle_mute.set_label("Unmute");
    else
        m_menu_dm_toggle_mute.set_label("Mute");
}

void ChannelList::OnThreadSubmenuPopup() {
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

ChannelList::type_signal_action_channel_item_select ChannelList::signal_action_channel_item_select() {
    return m_signal_action_channel_item_select;
}

ChannelList::type_signal_action_guild_leave ChannelList::signal_action_guild_leave() {
    return m_signal_action_guild_leave;
}

ChannelList::type_signal_action_guild_settings ChannelList::signal_action_guild_settings() {
    return m_signal_action_guild_settings;
}

#ifdef WITH_LIBHANDY
ChannelList::type_signal_action_open_new_tab ChannelList::signal_action_open_new_tab() {
    return m_signal_action_open_new_tab;
}
#endif

ChannelList::ModelColumns::ModelColumns() {
    add(m_type);
    add(m_id);
    add(m_name);
    add(m_icon);
    add(m_icon_anim);
    add(m_sort);
    add(m_nsfw);
    add(m_expanded);
    add(m_color);
}
