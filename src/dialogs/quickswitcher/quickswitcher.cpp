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
}

void QuickSwitcher::IndexPrivateChannels() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    for (auto &dm_id : discord.GetPrivateChannels()) {
        if (auto dm = discord.GetChannel(dm_id); dm.has_value()) {
            const auto sort = static_cast<uint64_t>(-(dm->LastMessageID.has_value() ? *dm->LastMessageID : dm_id));
            m_index.push_back({ dm->GetDisplayName(), sort, dm_id });
        }
    }
}

void QuickSwitcher::IndexChannels() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto channels = discord.GetAllChannelData();
    for (auto &channel : channels) {
        if (!channel.Name.has_value()) continue;
        m_index.push_back({ *channel.Name, static_cast<uint64_t>(channel.ID), channel.ID });
    }
}

void QuickSwitcher::Search() {
    for (auto child : m_results.get_children()) delete child;

    const auto query = m_entry.get_text();
    if (query.empty()) return;

    std::vector<SwitcherEntry> results;
    for (auto &item : m_index) {
        if (StringContainsCaseless(item.Name, query)) {
            results.push_back(item);
        }
    }

    std::sort(results.begin(), results.end(), [](const SwitcherEntry &a, const SwitcherEntry &b) -> bool {
        return a.Sort < b.Sort;
    });

    for (auto &result : results) {
        auto *row = Gtk::make_managed<QuickSwitcherResultRow>(result.ID);
        auto *lbl = Gtk::make_managed<Gtk::Label>(result.Name);
        row->add(*lbl);
        lbl->show();
        row->show();
        m_results.add(*row);
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

void QuickSwitcher::OnEntryActivate() {
    if (auto *row = dynamic_cast<QuickSwitcherResultRow *>(m_results.get_selected_row())) {
        Abaddon::Get().ActionChannelOpened(row->ID, false);
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
        Abaddon::Get().ActionChannelOpened(row->ID, false);
    }
    response(Gtk::RESPONSE_OK);
}
