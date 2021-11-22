#include "joinguild.hpp"
#include "abaddon.hpp"
#include <nlohmann/json.hpp>
#include <regex>

JoinGuildDialog::JoinGuildDialog(Gtk::Window &parent)
    : Gtk::Dialog("Join Server", parent, true)
    , m_layout(Gtk::ORIENTATION_VERTICAL)
    , m_ok("OK")
    , m_cancel("Cancel")
    , m_info("Enter code") {
    set_default_size(300, 50);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");

    Glib::signal_idle().connect(sigc::mem_fun(*this, &JoinGuildDialog::on_idle_slot));

    m_entry.signal_changed().connect(sigc::mem_fun(*this, &JoinGuildDialog::on_entry_changed));

    m_ok.set_sensitive(false);

    m_ok.signal_clicked().connect([&]() {
        response(Gtk::RESPONSE_OK);
    });

    m_cancel.signal_clicked().connect([&]() {
        response(Gtk::RESPONSE_CANCEL);
    });

    m_entry.set_hexpand(true);
    m_layout.add(m_entry);
    m_lower.set_hexpand(true);
    m_lower.pack_start(m_info);
    m_info.set_halign(Gtk::ALIGN_START);
    m_lower.pack_start(m_ok, Gtk::PACK_SHRINK);
    m_lower.pack_start(m_cancel, Gtk::PACK_SHRINK);
    m_ok.set_halign(Gtk::ALIGN_END);
    m_cancel.set_halign(Gtk::ALIGN_END);
    m_layout.add(m_lower);
    get_content_area()->add(m_layout);

    show_all_children();
}

void JoinGuildDialog::on_entry_changed() {
    std::string s = m_entry.get_text();
    std::regex invite_regex(R"((https?:\/\/)?discord\.(gg(\/invite)?\/|com\/invite\/)([A-Za-z0-9\-]+))", std::regex_constants::ECMAScript);
    std::smatch match;
    bool full_url = std::regex_search(s, match, invite_regex);
    if (full_url || IsCode(s)) {
        m_code = full_url ? match[4].str() : s;
        m_needs_request = true;
        m_ok.set_sensitive(false);
    } else {
        m_ok.set_sensitive(false);
    }
}

void JoinGuildDialog::CheckCode() {
    auto cb = [this](const std::optional<InviteData> &invite) {
        if (invite.has_value()) {
            m_ok.set_sensitive(true);
            if (invite->Guild.has_value()) {
                if (invite->MemberCount.has_value())
                    m_info.set_text(invite->Guild->Name + " (" + std::to_string(*invite->MemberCount) + " members)");
                else
                    m_info.set_text(invite->Guild->Name);
            } else {
                m_info.set_text("Group DM (" + std::to_string(*invite->MemberCount) + " members)");
            }
        } else {
            m_ok.set_sensitive(false);
            m_info.set_text("Invalid invite");
        }
    };
    Abaddon::Get().GetDiscordClient().FetchInvite(m_code, sigc::track_obj(cb, *this));
}

bool JoinGuildDialog::IsCode(std::string str) {
    return str.length() >= 2 && std::all_of(str.begin(), str.end(), [](char c) -> bool { return std::isalnum(c) || c == '-'; });
}

std::string JoinGuildDialog::GetCode() {
    return m_code;
}

static const constexpr int RateLimitMS = 1500;
bool JoinGuildDialog::on_idle_slot() {
    const auto now = std::chrono::steady_clock::now();
    if (m_needs_request && ((now - m_last_req_time) > std::chrono::milliseconds(RateLimitMS))) {
        m_needs_request = false;
        m_last_req_time = now;
        CheckCode();
    }

    return true;
}
