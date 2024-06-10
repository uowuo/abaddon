#include "quickswitcher.hpp"

#include "abaddon.hpp"
#include "quickswitcherresultrow.hpp"
#include "util.hpp"

QuickSwitcher::QuickSwitcher(Gtk::Window &parent)
    : Gtk::Dialog("Quick Switcher", parent, true) {
    Index();

    set_decorated(false);
    set_default_size(350, 175);

    m_entry.set_placeholder_text("Where would you like to go?");
    m_entry.set_hexpand(true);
    m_entry.signal_stop_search().connect([this]() {
        response(Gtk::RESPONSE_OK);
    });
    m_entry.signal_activate().connect(sigc::mem_fun(*this, &QuickSwitcher::OnEntryActivate));
    m_entry.signal_changed().connect(sigc::mem_fun(*this, &QuickSwitcher::Search));
    m_entry.signal_previous_match().connect([this]() { GoDown(); });
    m_entry.signal_next_match().connect([this]() { GoUp(); });
    m_entry.signal_key_press_event().connect(sigc::mem_fun(*this, &QuickSwitcher::OnEntryKeyPress), false);
    get_content_area()->add(m_entry);

    m_results.set_activate_on_single_click(true);
    m_results.set_selection_mode(Gtk::SELECTION_SINGLE);
    m_results.set_vexpand(true);
    m_results.signal_row_activated().connect(sigc::mem_fun(*this, &QuickSwitcher::OnResultRowActivate));
    m_results_scroll.add(m_results);
    get_content_area()->add(m_results_scroll);

    show_all_children();
}

void QuickSwitcher::Index() {
    m_index.clear();
    IndexPrivateChannels();
    IndexChannels();
    IndexGuilds();
}

void QuickSwitcher::IndexPrivateChannels() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    for (auto &dm_id : discord.GetPrivateChannels()) {
        if (auto dm = discord.GetChannel(dm_id); dm.has_value()) {
            const auto sort = static_cast<uint64_t>(-(dm->LastMessageID.has_value() ? *dm->LastMessageID : dm_id));
            m_index[dm_id] = { SwitcherEntry::ResultType::DM,
                               dm->GetDisplayName(),
                               sort,
                               dm_id };
        }
    }
}

void QuickSwitcher::IndexChannels() {
    auto &discord = Abaddon::Get().GetDiscordClient();

    const auto channels = discord.GetAllChannelData();
    // grab literally everything to do in memory otherwise we get a shit ton of IOs
    auto overwrites = discord.GetAllPermissionOverwrites();

    auto member_roles = discord.GetAllMemberRoles(discord.GetUserData().ID);
    std::unordered_map<Snowflake, RoleData> roles;
    for (const auto &[guild_id, guild_roles] : member_roles) {
        for (const auto &role_data : guild_roles) {
            roles.emplace(role_data.ID, role_data);
        }
    }

    for (auto &channel : channels) {
        if (!channel.Name.has_value()) continue;
        if (!channel.IsText()) continue;
        if (channel.GuildID.has_value() &&
            !discord.HasSelfChannelPermission(channel, Permission::VIEW_CHANNEL, roles, member_roles[*channel.GuildID], overwrites[channel.ID])) continue;
        m_index[channel.ID] = { SwitcherEntry::ResultType::Channel,
                                *channel.Name,
                                static_cast<uint64_t>(channel.ID),
                                channel.ID };
    }
}

void QuickSwitcher::IndexGuilds() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto guilds = discord.GetGuilds();
    for (auto guild_id : guilds) {
        const auto guild = discord.GetGuild(guild_id);
        if (!guild.has_value()) continue;
        // todo make this smart
        m_index[guild->ID] = { SwitcherEntry::ResultType::Guild,
                               guild->Name,
                               static_cast<uint64_t>(guild->ID),
                               guild->ID };
    }
}

void QuickSwitcher::Search() {
    for (auto child : m_results.get_children()) delete child;

    const auto query = m_entry.get_text();
    if (query.empty()) return;

    std::vector<SwitcherEntry> results;
    for (auto &[id, item] : m_index) {
        if (StringContainsCaseless(item.Name, query)) {
            results.push_back(item);
        }
    }

    std::sort(results.begin(), results.end(), [](const SwitcherEntry &a, const SwitcherEntry &b) -> bool {
        return a.Sort < b.Sort;
    });

    auto &discord = Abaddon::Get().GetDiscordClient();

    int result_count = 0;
    const int MAX_RESULTS = 15;
    for (auto &result : results) {
        QuickSwitcherResultRow *row = nullptr;
        switch (result.Type) {
            case SwitcherEntry::ResultType::DM: {
                if (const auto channel = discord.GetChannel(result.ID); channel.has_value()) {
                    row = Gtk::make_managed<QuickSwitcherResultRowDM>(*channel);
                }
            } break;
            case SwitcherEntry::ResultType::Channel: {
                if (const auto channel = discord.GetChannel(result.ID); channel.has_value()) {
                    row = Gtk::make_managed<QuickSwitcherResultRowChannel>(*channel);
                }
            } break;
            case SwitcherEntry::ResultType::Guild: {
                if (const auto guild = discord.GetGuild(result.ID); guild.has_value()) {
                    row = Gtk::make_managed<QuickSwitcherResultRowGuild>(*guild);
                }
            } break;
        }
        if (row != nullptr) {
            row->show();
            m_results.add(*row);
            if (++result_count >= MAX_RESULTS) break;
        }
    }

    GoUp();
}

void QuickSwitcher::GoUp() {
    Move(-1);
}

void QuickSwitcher::GoDown() {
    Move(1);
}

void QuickSwitcher::Move(int dir) {
    auto children = m_results.get_children();
    if (children.empty()) return;

    auto selected = m_results.get_selected_row();
    if (selected == nullptr) {
        if (auto row = dynamic_cast<Gtk::ListBoxRow *>(children[0])) {
            m_results.select_row(*row);
        }
        return;
    }

    int idx = selected->get_index() + dir;
    if (idx < 0) {
        idx = children.size() - 1;
    } else if (idx >= children.size()) {
        idx = 0;
    }

    if (auto row = dynamic_cast<Gtk::ListBoxRow *>(children[idx])) {
        m_results.select_row(*row);
    }

    ScrollListBoxToSelected(m_results);
}

void QuickSwitcher::AcceptResult(Snowflake id) {
    const auto result = m_index.find(id);
    if (result == m_index.end()) return;
    const auto &entry = result->second;

    switch (entry.Type) {
        case SwitcherEntry::ResultType::Channel:
        case SwitcherEntry::ResultType::DM:
            Abaddon::Get().ActionChannelOpened(entry.ID, false);
            break;
        case SwitcherEntry::ResultType::Guild: {
            const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(entry.ID);
            if (!guild.has_value()) return;
            const auto channel = guild->GetDefaultTextChannel();
            if (channel.has_value()) {
                Abaddon::Get().ActionChannelOpened(*channel, false);
            }
        } break;
    }
}

void QuickSwitcher::OnEntryActivate() {
    if (auto *row = dynamic_cast<QuickSwitcherResultRow *>(m_results.get_selected_row())) {
        AcceptResult(row->ID);
    }
    response(Gtk::RESPONSE_OK);
}

bool QuickSwitcher::OnEntryKeyPress(GdkEventKey *event) {
    if (event->type != GDK_KEY_PRESS) return false;
    switch (event->keyval) {
        case GDK_KEY_Up:
            GoUp();
            return true;
        case GDK_KEY_Down:
            GoDown();
            return true;
        default:
            return false;
    }
}

void QuickSwitcher::OnResultRowActivate(Gtk::ListBoxRow *row_) {
    if (auto *row = dynamic_cast<QuickSwitcherResultRow *>(row_)) {
        AcceptResult(row->ID);
    }
    response(Gtk::RESPONSE_OK);
}
