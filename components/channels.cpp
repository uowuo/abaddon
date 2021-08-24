#include "channels.hpp"
#include <algorithm>
#include <map>
#include <unordered_map>
#include "../abaddon.hpp"
#include "../imgmanager.hpp"
#include "../util.hpp"
#include "statusindicator.hpp"

ChannelList::ChannelList()
    : Glib::ObjectBase(typeid(ChannelList))
    , Gtk::ScrolledWindow()
    , m_model(Gtk::TreeStore::create(m_columns))
    , m_menu_guild_copy_id("_Copy ID", true)
    , m_menu_guild_settings("View _Settings", true)
    , m_menu_guild_leave("_Leave", true)
    , m_menu_category_copy_id("_Copy ID", true)
    , m_menu_channel_copy_id("_Copy ID", true)
    , m_menu_dm_close("") // changes depending on if group or not
    , m_menu_dm_copy_id("_Copy ID", true)
    , m_menu_thread_copy_id("_Copy ID", true)
    , m_menu_thread_leave("_Leave", true) {
    get_style_context()->add_class("channel-list");

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
            m_signal_action_channel_item_select.emit(static_cast<Snowflake>(row[m_columns.m_id]));
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
    column->add_attribute(renderer->property_expanded(), m_columns.m_expanded);
    column->add_attribute(renderer->property_nsfw(), m_columns.m_nsfw);
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
    m_menu_guild.append(m_menu_guild_copy_id);
    m_menu_guild.append(m_menu_guild_settings);
    m_menu_guild.append(m_menu_guild_leave);
    m_menu_guild.show_all();

    m_menu_category_copy_id.signal_activate().connect([this] {
        Gtk::Clipboard::get()->set_text(std::to_string((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });
    m_menu_category.append(m_menu_category_copy_id);
    m_menu_category.show_all();

    m_menu_channel_copy_id.signal_activate().connect([this] {
        Gtk::Clipboard::get()->set_text(std::to_string((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });
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
    m_menu_dm.append(m_menu_dm_copy_id);
    m_menu_dm.append(m_menu_dm_close);
    m_menu_dm.show_all();

    m_menu_thread_copy_id.signal_activate().connect([this] {
        Gtk::Clipboard::get()->set_text(std::to_string((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]));
    });
    m_menu_thread_leave.signal_activate().connect([this] {
        if (Abaddon::Get().ShowConfirm("Are you sure you want to leave this thread?"))
            Abaddon::Get().GetDiscordClient().LeaveThread(static_cast<Snowflake>((*m_model->get_iter(m_path_for_menu))[m_columns.m_id]), "Context%20Menu", [](...) {});
    });
    m_menu_thread.append(m_menu_thread_copy_id);
    m_menu_thread.append(m_menu_thread_leave);
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
}

void ChannelList::UpdateListing() {
    m_updating_listing = true;

    m_model->clear();

    auto &discord = Abaddon::Get().GetDiscordClient();
    auto &img = Abaddon::Get().GetImageManager();

    const auto guild_ids = discord.GetUserSortedGuilds();
    int sortnum = 0;
    for (const auto &guild_id : guild_ids) {
        const auto guild = discord.GetGuild(guild_id);
        if (!guild.has_value()) continue;

        auto iter = AddGuild(*guild);
        (*iter)[m_columns.m_sort] = sortnum++;
    }

    m_updating_listing = false;

    AddPrivateChannels();
}

void ChannelList::UpdateNewGuild(const GuildData &guild) {
    auto &img = Abaddon::Get().GetImageManager();

    auto iter = AddGuild(guild);

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
    if (!IsTextChannel(channel->Type)) return;

    // refresh stuff that might have changed
    const bool is_orphan_TMP = !channel->ParentID.has_value();
    (*iter)[m_columns.m_name] = "#" + Glib::Markup::escape_text(*channel->Name);
    (*iter)[m_columns.m_nsfw] = channel->NSFW();
    (*iter)[m_columns.m_sort] = *channel->Position + (is_orphan_TMP ? OrphanChannelSortOffset : 0);

    // check if the parent has changed
    Gtk::TreeModel::iterator new_parent;
    if (channel->ParentID.has_value())
        new_parent = GetIteratorForChannelFromID(*channel->ParentID);
    else
        new_parent = GetIteratorForGuildFromID(*channel->GuildID);

    if (new_parent && iter->parent() != new_parent)
        MoveRow(iter, new_parent);
}

void ChannelList::UpdateCreateChannel(const ChannelData &channel) {
    ;
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

    static const bool show_animations = Abaddon::Get().GetSettings().GetShowAnimations();

    (*iter)[m_columns.m_name] = "<b>" + Glib::Markup::escape_text(guild->Name) + "</b>";
    (*iter)[m_columns.m_icon] = img.GetPlaceholder(GuildIconSize);
    if (show_animations && guild->HasAnimatedIcon()) {
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
    std::queue<Gtk::TreeModel::iterator> queue;
    queue.push(guild_iter);

    while (!queue.empty()) {
        auto item = queue.front();
        queue.pop();
        if ((*item)[m_columns.m_type] == RenderType::Thread)
            threads.push_back(static_cast<Snowflake>((*item)[m_columns.m_id]));
        for (auto child : item->children())
            queue.push(child);
    }

    // delete all threads not present in the synced data
    for (auto thread_id : threads) {
        if (std::find_if(data.Threads.begin(), data.Threads.end(), [thread_id](const auto &x) { return x.ID == thread_id; }) == data.Threads.end()) {
            auto iter = GetIteratorForChannelFromID(thread_id);
            m_model->erase(iter);
        }
    }
}

void ChannelList::DeleteThreadRow(Snowflake id) {
    auto iter = GetIteratorForChannelFromID(id);
    if (iter)
        m_model->erase(iter);
}

// create a temporary channel row for non-joined threads
// and delete them when the active channel switches off of them if still not joined
void ChannelList::SetActiveChannel(Snowflake id) {
    if (m_temporary_thread_row) {
        const auto thread_id = static_cast<Snowflake>((*m_temporary_thread_row)[m_columns.m_id]);
        const auto thread = Abaddon::Get().GetDiscordClient().GetChannel(thread_id);
        if (thread.has_value() && (!thread->IsJoinedThread() || thread->ThreadMetadata->IsArchived))
            m_model->erase(m_temporary_thread_row);
        m_temporary_thread_row = {};
    }

    const auto channel_iter = GetIteratorForChannelFromID(id);
    if (channel_iter) {
        m_view.expand_to_path(m_model->get_path(channel_iter));
        m_view.get_selection()->select(channel_iter);
    } else {
        m_view.get_selection()->unselect_all();
        // SetActiveChannel should probably just take the channel object
        const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(id);
        if (!channel.has_value() || !channel->IsThread()) return;
        auto parent_iter = GetIteratorForChannelFromID(*channel->ParentID);
        if (!parent_iter) return;
        m_temporary_thread_row = CreateThreadRow(parent_iter->children(), *channel);
        m_view.get_selection()->select(m_temporary_thread_row);
    }
}

Gtk::TreeModel::iterator ChannelList::AddGuild(const GuildData &guild) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto &img = Abaddon::Get().GetImageManager();

    auto guild_row = *m_model->append();
    guild_row[m_columns.m_type] = RenderType::Guild;
    guild_row[m_columns.m_id] = guild.ID;
    guild_row[m_columns.m_name] = "<b>" + Glib::Markup::escape_text(guild.Name) + "</b>";
    guild_row[m_columns.m_icon] = img.GetPlaceholder(GuildIconSize);

    static const bool show_animations = Abaddon::Get().GetSettings().GetShowAnimations();

    if (show_animations && guild.HasAnimatedIcon()) {
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
    const auto add_threads = [&](const ChannelData &channel, Gtk::TreeRow row) {
        row[m_columns.m_expanded] = true;

        const auto it = threads.find(channel.ID);
        if (it == threads.end()) return;

        for (const auto &thread : it->second)
            CreateThreadRow(row.children(), thread);
    };

    for (const auto &channel : orphan_channels) {
        auto channel_row = *m_model->append(guild_row.children());
        channel_row[m_columns.m_type] = RenderType::TextChannel;
        channel_row[m_columns.m_id] = channel.ID;
        channel_row[m_columns.m_name] = "#" + Glib::Markup::escape_text(*channel.Name);
        channel_row[m_columns.m_sort] = *channel.Position + OrphanChannelSortOffset;
        channel_row[m_columns.m_nsfw] = channel.NSFW();
        add_threads(channel, channel_row);
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
        // m_view.expand_row wont work because it might not have channels

        for (const auto &channel : channels) {
            auto channel_row = *m_model->append(cat_row.children());
            channel_row[m_columns.m_type] = RenderType::TextChannel;
            channel_row[m_columns.m_id] = channel.ID;
            channel_row[m_columns.m_name] = "#" + Glib::Markup::escape_text(*channel.Name);
            channel_row[m_columns.m_sort] = *channel.Position;
            channel_row[m_columns.m_nsfw] = channel.NSFW();
            add_threads(channel, channel_row);
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
    thread_row[m_columns.m_sort] = channel.ID;
    thread_row[m_columns.m_nsfw] = false;

    return thread_iter;
}

void ChannelList::UpdateChannelCategory(const ChannelData &channel) {
    auto iter = GetIteratorForChannelFromID(channel.ID);
    if (!iter) return;

    (*iter)[m_columns.m_sort] = *channel.Position;
    (*iter)[m_columns.m_name] = Glib::Markup::escape_text(*channel.Name);
}

Gtk::TreeModel::iterator ChannelList::GetIteratorForGuildFromID(Snowflake id) {
    for (const auto child : m_model->children()) {
        if (child[m_columns.m_id] == id)
            return child;
    }
    return {};
}

Gtk::TreeModel::iterator ChannelList::GetIteratorForChannelFromID(Snowflake id) {
    std::queue<Gtk::TreeModel::iterator> queue;
    for (const auto child : m_model->children())
        for (const auto child2 : child.children())
            queue.push(child2);

    while (!queue.empty()) {
        auto item = queue.front();
        if ((*item)[m_columns.m_id] == id) return item;
        for (const auto child : item->children())
            queue.push(child);
        queue.pop();
    }

    return {};
}

bool ChannelList::IsTextChannel(ChannelType type) {
    return type == ChannelType::GUILD_TEXT || type == ChannelType::GUILD_NEWS;
}

// this should be unncessary but something is behaving strange so its just in case
void ChannelList::OnRowCollapsed(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path) {
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
        if (recipients.size() > 0)
            top_recipient = recipients[0];

        auto iter = m_model->append(header_row->children());
        auto row = *iter;
        row[m_columns.m_type] = RenderType::DM;
        row[m_columns.m_id] = dm_id;
        row[m_columns.m_sort] = -(dm->LastMessageID.has_value() ? *dm->LastMessageID : dm_id);
        row[m_columns.m_icon] = img.GetPlaceholder(DMIconSize);

        if (dm->Type == ChannelType::DM && top_recipient.has_value())
            row[m_columns.m_name] = Glib::Markup::escape_text(top_recipient->Username);
        else if (dm->Type == ChannelType::GROUP_DM)
            row[m_columns.m_name] = std::to_string(recipients.size()) + " members";

        if (top_recipient.has_value()) {
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
    if (recipients.size() > 0)
        top_recipient = recipients[0];

    auto iter = m_model->append(header_row->children());
    auto row = *iter;
    row[m_columns.m_type] = RenderType::DM;
    row[m_columns.m_id] = dm.ID;
    row[m_columns.m_sort] = -(dm.LastMessageID.has_value() ? *dm.LastMessageID : dm.ID);
    row[m_columns.m_icon] = img.GetPlaceholder(DMIconSize);

    if (dm.Type == ChannelType::DM && top_recipient.has_value())
        row[m_columns.m_name] = Glib::Markup::escape_text(top_recipient->Username);
    else if (dm.Type == ChannelType::GROUP_DM)
        row[m_columns.m_name] = std::to_string(recipients.size()) + " members";

    if (top_recipient.has_value()) {
        const auto cb = [this, iter](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            if (iter)
                (*iter)[m_columns.m_icon] = pb->scale_simple(DMIconSize, DMIconSize, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL(top_recipient->GetAvatarURL("png", "32"), sigc::track_obj(cb, *this));
    }
}

void ChannelList::OnMessageCreate(const Message &msg) {
    const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(msg.ChannelID);
    if (!channel.has_value()) return;
    if (channel->Type != ChannelType::DM && channel->Type != ChannelType::GROUP_DM) return;
    auto iter = GetIteratorForChannelFromID(msg.ChannelID);
    if (iter)
        (*iter)[m_columns.m_sort] = -msg.ID;
}

bool ChannelList::OnButtonPressEvent(GdkEventButton *ev) {
    if (ev->button == GDK_BUTTON_SECONDARY && ev->type == GDK_BUTTON_PRESS) {
        if (m_view.get_path_at_pos(ev->x, ev->y, m_path_for_menu)) {
            auto row = (*m_model->get_iter(m_path_for_menu));
            switch (static_cast<RenderType>(row[m_columns.m_type])) {
                case RenderType::Guild:
                    m_menu_guild.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                    break;
                case RenderType::Category:
                    m_menu_category.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                    break;
                case RenderType::TextChannel:
                    m_menu_channel.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                    break;
                case RenderType::DM: {
                    const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(static_cast<Snowflake>(row[m_columns.m_id]));
                    if (channel.has_value()) {
                        m_menu_dm_close.set_label(channel->Type == ChannelType::DM ? "Close" : "Leave");
                        m_menu_dm_close.show();
                    } else
                        m_menu_dm_close.hide();
                    m_menu_dm.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                } break;
                case RenderType::Thread: {
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

ChannelList::type_signal_action_channel_item_select ChannelList::signal_action_channel_item_select() {
    return m_signal_action_channel_item_select;
}

ChannelList::type_signal_action_guild_leave ChannelList::signal_action_guild_leave() {
    return m_signal_action_guild_leave;
}

ChannelList::type_signal_action_guild_settings ChannelList::signal_action_guild_settings() {
    return m_signal_action_guild_settings;
}

ChannelList::ModelColumns::ModelColumns() {
    add(m_type);
    add(m_id);
    add(m_name);
    add(m_icon);
    add(m_icon_anim);
    add(m_sort);
    add(m_nsfw);
    add(m_expanded);
}

CellRendererChannels::CellRendererChannels()
    : Glib::ObjectBase(typeid(CellRendererChannels))
    , Gtk::CellRenderer()
    , m_property_type(*this, "render-type")
    , m_property_name(*this, "name")
    , m_property_pixbuf(*this, "pixbuf")
    , m_property_pixbuf_animation(*this, "pixbuf-animation")
    , m_property_expanded(*this, "expanded")
    , m_property_nsfw(*this, "nsfw") {
    property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
    property_xpad() = 2;
    property_ypad() = 2;
    m_property_name.get_proxy().signal_changed().connect([this] {
        m_renderer_text.property_markup() = m_property_name;
    });
}

CellRendererChannels::~CellRendererChannels() {
}

Glib::PropertyProxy<RenderType> CellRendererChannels::property_type() {
    return m_property_type.get_proxy();
}

Glib::PropertyProxy<Glib::ustring> CellRendererChannels::property_name() {
    return m_property_name.get_proxy();
}

Glib::PropertyProxy<Glib::RefPtr<Gdk::Pixbuf>> CellRendererChannels::property_icon() {
    return m_property_pixbuf.get_proxy();
}

Glib::PropertyProxy<Glib::RefPtr<Gdk::PixbufAnimation>> CellRendererChannels::property_icon_animation() {
    return m_property_pixbuf_animation.get_proxy();
}

Glib::PropertyProxy<bool> CellRendererChannels::property_expanded() {
    return m_property_expanded.get_proxy();
}

Glib::PropertyProxy<bool> CellRendererChannels::property_nsfw() {
    return m_property_nsfw.get_proxy();
}

void CellRendererChannels::get_preferred_width_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_width_vfunc_guild(widget, minimum_width, natural_width);
        case RenderType::Category:
            return get_preferred_width_vfunc_category(widget, minimum_width, natural_width);
        case RenderType::TextChannel:
            return get_preferred_width_vfunc_channel(widget, minimum_width, natural_width);
        case RenderType::Thread:
            return get_preferred_width_vfunc_thread(widget, minimum_width, natural_width);
        case RenderType::DMHeader:
            return get_preferred_width_vfunc_dmheader(widget, minimum_width, natural_width);
        case RenderType::DM:
            return get_preferred_width_vfunc_dm(widget, minimum_width, natural_width);
    }
}

void CellRendererChannels::get_preferred_width_for_height_vfunc(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_width_for_height_vfunc_guild(widget, height, minimum_width, natural_width);
        case RenderType::Category:
            return get_preferred_width_for_height_vfunc_category(widget, height, minimum_width, natural_width);
        case RenderType::TextChannel:
            return get_preferred_width_for_height_vfunc_channel(widget, height, minimum_width, natural_width);
        case RenderType::Thread:
            return get_preferred_width_for_height_vfunc_thread(widget, height, minimum_width, natural_width);
        case RenderType::DMHeader:
            return get_preferred_width_for_height_vfunc_dmheader(widget, height, minimum_width, natural_width);
        case RenderType::DM:
            return get_preferred_width_for_height_vfunc_dm(widget, height, minimum_width, natural_width);
    }
}

void CellRendererChannels::get_preferred_height_vfunc(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_height_vfunc_guild(widget, minimum_height, natural_height);
        case RenderType::Category:
            return get_preferred_height_vfunc_category(widget, minimum_height, natural_height);
        case RenderType::TextChannel:
            return get_preferred_height_vfunc_channel(widget, minimum_height, natural_height);
        case RenderType::Thread:
            return get_preferred_height_vfunc_thread(widget, minimum_height, natural_height);
        case RenderType::DMHeader:
            return get_preferred_height_vfunc_dmheader(widget, minimum_height, natural_height);
        case RenderType::DM:
            return get_preferred_height_vfunc_dm(widget, minimum_height, natural_height);
    }
}

void CellRendererChannels::get_preferred_height_for_width_vfunc(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_height_for_width_vfunc_guild(widget, width, minimum_height, natural_height);
        case RenderType::Category:
            return get_preferred_height_for_width_vfunc_category(widget, width, minimum_height, natural_height);
        case RenderType::TextChannel:
            return get_preferred_height_for_width_vfunc_channel(widget, width, minimum_height, natural_height);
        case RenderType::Thread:
            return get_preferred_height_for_width_vfunc_thread(widget, width, minimum_height, natural_height);
        case RenderType::DMHeader:
            return get_preferred_height_for_width_vfunc_dmheader(widget, width, minimum_height, natural_height);
        case RenderType::DM:
            return get_preferred_height_for_width_vfunc_dm(widget, width, minimum_height, natural_height);
    }
}

void CellRendererChannels::render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return render_vfunc_guild(cr, widget, background_area, cell_area, flags);
        case RenderType::Category:
            return render_vfunc_category(cr, widget, background_area, cell_area, flags);
        case RenderType::TextChannel:
            return render_vfunc_channel(cr, widget, background_area, cell_area, flags);
        case RenderType::Thread:
            return render_vfunc_thread(cr, widget, background_area, cell_area, flags);
        case RenderType::DMHeader:
            return render_vfunc_dmheader(cr, widget, background_area, cell_area, flags);
        case RenderType::DM:
            return render_vfunc_dm(cr, widget, background_area, cell_area, flags);
    }
}

// guild functions

void CellRendererChannels::get_preferred_width_vfunc_guild(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    int pixbuf_width = 0;

    if (auto pixbuf = m_property_pixbuf_animation.get_value())
        pixbuf_width = pixbuf->get_width();
    else if (auto pixbuf = m_property_pixbuf.get_value())
        pixbuf_width = pixbuf->get_width();

    int text_min, text_nat;
    m_renderer_text.get_preferred_width(widget, text_min, text_nat);

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_width = std::max(text_min, pixbuf_width) + xpad * 2;
    natural_width = std::max(text_nat, pixbuf_width) + xpad * 2;
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_guild(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc_guild(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_guild(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    int pixbuf_height = 0;
    if (auto pixbuf = m_property_pixbuf_animation.get_value())
        pixbuf_height = pixbuf->get_height();
    else if (auto pixbuf = m_property_pixbuf.get_value())
        pixbuf_height = pixbuf->get_height();

    int text_min, text_nat;
    m_renderer_text.get_preferred_height(widget, text_min, text_nat);

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_height = std::max(text_min, pixbuf_height) + ypad * 2;
    natural_height = std::max(text_nat, pixbuf_height) + ypad * 2;
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_guild(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc_guild(widget, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_guild(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    Gtk::Requisition minimum, natural;
    get_preferred_size(widget, minimum, natural);

    int pixbuf_w, pixbuf_h = 0;
    if (auto pixbuf = m_property_pixbuf_animation.get_value()) {
        pixbuf_w = pixbuf->get_width();
        pixbuf_h = pixbuf->get_height();
    } else if (auto pixbuf = m_property_pixbuf.get_value()) {
        pixbuf_w = pixbuf->get_width();
        pixbuf_h = pixbuf->get_height();
    }

    const double icon_w = pixbuf_w;
    const double icon_h = pixbuf_h;
    const double icon_x = background_area.get_x();
    const double icon_y = background_area.get_y() + background_area.get_height() / 2.0 - icon_h / 2.0;

    const double text_x = icon_x + icon_w + 5.0;
    const double text_y = background_area.get_y() + background_area.get_height() / 2.0 - text_natural.height / 2.0;
    const double text_w = text_natural.width;
    const double text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);

    const static bool hover_only = Abaddon::Get().GetSettings().GetAnimatedGuildHoverOnly();
    const bool is_hovered = flags & Gtk::CELL_RENDERER_PRELIT;
    auto anim = m_property_pixbuf_animation.get_value();

    // kinda gross
    if (anim) {
        auto map_iter = m_pixbuf_anim_iters.find(anim);
        if (map_iter == m_pixbuf_anim_iters.end())
            m_pixbuf_anim_iters[anim] = anim->get_iter(nullptr);
        auto pb_iter = m_pixbuf_anim_iters.at(anim);

        const auto cb = [this, &widget, anim, icon_x, icon_y, icon_w, icon_h] {
            if (m_pixbuf_anim_iters.at(anim)->advance())
                widget.queue_draw_area(icon_x, icon_y, icon_w, icon_h);
        };

        if ((hover_only && is_hovered) || !hover_only)
            Glib::signal_timeout().connect_once(sigc::track_obj(cb, widget), pb_iter->get_delay_time());
        if (hover_only && !is_hovered)
            m_pixbuf_anim_iters[anim] = anim->get_iter(nullptr);

        Gdk::Cairo::set_source_pixbuf(cr, pb_iter->get_pixbuf(), icon_x, icon_y);
        cr->rectangle(icon_x, icon_y, icon_w, icon_h);
        cr->fill();
    } else if (auto pixbuf = m_property_pixbuf.get_value()) {
        Gdk::Cairo::set_source_pixbuf(cr, pixbuf, icon_x, icon_y);
        cr->rectangle(icon_x, icon_y, icon_w, icon_h);
        cr->fill();
    }
}

// category

void CellRendererChannels::get_preferred_width_vfunc_category(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_category(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_category(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_category(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_category(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    int available_xpad = background_area.get_width();
    int available_ypad = background_area.get_height();

    // todo: figure out how Gtk::Arrow is rendered because i like it better :^)
    constexpr static int len = 5;
    int x1, y1, x2, y2, x3, y3;
    if (property_expanded()) {
        x1 = background_area.get_x() + 7;
        y1 = background_area.get_y() + background_area.get_height() / 2 - len;
        x2 = background_area.get_x() + 7 + len;
        y2 = background_area.get_y() + background_area.get_height() / 2 + len;
        x3 = background_area.get_x() + 7 + len * 2;
        y3 = background_area.get_y() + background_area.get_height() / 2 - len;
    } else {
        x1 = background_area.get_x() + 7;
        y1 = background_area.get_y() + background_area.get_height() / 2 - len;
        x2 = background_area.get_x() + 7 + len * 2;
        y2 = background_area.get_y() + background_area.get_height() / 2;
        x3 = background_area.get_x() + 7;
        y3 = background_area.get_y() + background_area.get_height() / 2 + len;
    }
    cr->move_to(x1, y1);
    cr->line_to(x2, y2);
    cr->line_to(x3, y3);
    static const auto expander_color = Gdk::RGBA(Abaddon::Get().GetSettings().GetChannelsExpanderColor());
    cr->set_source_rgb(expander_color.get_red(), expander_color.get_green(), expander_color.get_blue());
    cr->stroke();

    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    const int text_x = background_area.get_x() + 22;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - text_natural.height / 2;
    const int text_w = text_natural.width;
    const int text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
}

// text channel

void CellRendererChannels::get_preferred_width_vfunc_channel(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_channel(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_channel(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_channel(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_channel(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition minimum_size, natural_size;
    m_renderer_text.get_preferred_size(widget, minimum_size, natural_size);

    const int text_x = background_area.get_x() + 21;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - natural_size.height / 2;
    const int text_w = natural_size.width;
    const int text_h = natural_size.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    const static auto nsfw_color = Gdk::RGBA(Abaddon::Get().GetSettings().GetNSFWChannelColor());
    if (m_property_nsfw.get_value())
        m_renderer_text.property_foreground_rgba() = nsfw_color;
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
    // setting property_foreground_rgba() sets this to true which makes non-nsfw cells use the property too which is bad
    // so unset it
    m_renderer_text.property_foreground_set() = false;
}

// thread

void CellRendererChannels::get_preferred_width_vfunc_thread(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_thread(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc_thread(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_thread(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_thread(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc_thread(widget, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_thread(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition minimum_size, natural_size;
    m_renderer_text.get_preferred_size(widget, minimum_size, natural_size);

    const int text_x = background_area.get_x() + 26;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - natural_size.height / 2;
    const int text_w = natural_size.width;
    const int text_h = natural_size.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
}

// dm header

void CellRendererChannels::get_preferred_width_vfunc_dmheader(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_dmheader(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_dmheader(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_dmheader(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_dmheader(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    // gdk::rectangle more like gdk::stupid
    Gdk::Rectangle text_cell_area(
        cell_area.get_x() + 9, cell_area.get_y(), // maybe theres a better way to align this ?
        cell_area.get_width(), cell_area.get_height());
    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
}

// dm (basically the same thing as guild)

void CellRendererChannels::get_preferred_width_vfunc_dm(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    int pixbuf_width = 0;
    if (auto pixbuf = m_property_pixbuf.get_value())
        pixbuf_width = pixbuf->get_width();

    int text_min, text_nat;
    m_renderer_text.get_preferred_width(widget, text_min, text_nat);

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_width = std::max(text_min, pixbuf_width) + xpad * 2;
    natural_width = std::max(text_nat, pixbuf_width) + xpad * 2;
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_dm(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc_guild(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_dm(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    int pixbuf_height = 0;
    if (auto pixbuf = m_property_pixbuf.get_value())
        pixbuf_height = pixbuf->get_height();

    int text_min, text_nat;
    m_renderer_text.get_preferred_height(widget, text_min, text_nat);

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_height = std::max(text_min, pixbuf_height) + ypad * 2;
    natural_height = std::max(text_nat, pixbuf_height) + ypad * 2;
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_dm(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc_guild(widget, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_dm(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    Gtk::Requisition minimum, natural;
    get_preferred_size(widget, minimum, natural);

    auto pixbuf = m_property_pixbuf.get_value();

    const double icon_w = pixbuf->get_width();
    const double icon_h = pixbuf->get_height();
    const double icon_x = background_area.get_x() + 2;
    const double icon_y = background_area.get_y() + background_area.get_height() / 2.0 - icon_h / 2.0;

    const double text_x = icon_x + icon_w + 5.0;
    const double text_y = background_area.get_y() + background_area.get_height() / 2.0 - text_natural.height / 2.0;
    const double text_w = text_natural.width;
    const double text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);

    Gdk::Cairo::set_source_pixbuf(cr, m_property_pixbuf.get_value(), icon_x, icon_y);
    cr->rectangle(icon_x, icon_y, icon_w, icon_h);
    cr->fill();
}
