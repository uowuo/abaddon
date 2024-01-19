#include "userinfopane.hpp"

#include <unordered_set>

#include <gtkmm/messagedialog.h>

#include "abaddon.hpp"
#include "util.hpp"

static std::string GetConnectionURL(const ConnectionData &conn) {
    if (conn.Type == "github") {
        return "https://github.com/" + conn.Name;
    } else if (conn.Type == "steam") {
        return "https://steamcommunity.com/profiles/" + conn.ID;
    } else if (conn.Type == "twitch") {
        return "https://twitch.tv/" + conn.Name;
    } else if (conn.Type == "twitter") {
        return "https://twitter.com/i/user/" + conn.ID;
    } else if (conn.Type == "spotify") {
        return "https://open.spotify.com/user/" + conn.ID;
    } else if (conn.Type == "reddit") {
        return "https://reddit.com/u/" + conn.Name;
    } else if (conn.Type == "youtube") {
        return "https://www.youtube.com/channel/" + conn.ID;
    } else if (conn.Type == "facebook") {
        return "https://www.facebook.com/" + conn.ID;
    } else if (conn.Type == "ebay") {
        return "https://www.ebay.com/usr/" + conn.Name;
    } else if (conn.Type == "instagram") {
        return "https://www.instagram.com/" + conn.Name;
    } else if (conn.Type == "tiktok") {
        return "https://www.tiktok.com/@" + conn.Name;
    }

    return "";
}

ConnectionItem::ConnectionItem(const ConnectionData &conn)
    : m_box(Gtk::ORIENTATION_HORIZONTAL)
    , m_name(conn.Name) {
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    try {
        pixbuf = Gdk::Pixbuf::create_from_file(Abaddon::GetResPath("/" + conn.Type + ".png"), 32, 32);
    } catch (const Glib::Exception &e) {}
    std::string url = GetConnectionURL(conn);
    if (pixbuf) {
        m_image = Gtk::manage(new Gtk::Image(pixbuf));
        m_image->get_style_context()->add_class("profile-connection-image");
        m_box.add(*m_image);
    }
    m_box.set_halign(Gtk::ALIGN_START);
    m_box.set_size_request(200, -1);
    m_box.get_style_context()->add_class("profile-connection");
    m_name.get_style_context()->add_class("profile-connection-label");
    m_name.set_valign(Gtk::ALIGN_CENTER);
    m_name.set_single_line_mode(true);
    m_name.set_ellipsize(Pango::ELLIPSIZE_END);
    m_box.add(m_name);
    if (!url.empty()) {
        auto cb = [url](GdkEventButton *event) -> bool {
            if (event->type == GDK_BUTTON_RELEASE && event->button == GDK_BUTTON_PRIMARY) {
                LaunchBrowser(url);
                return true;
            }
            return false;
        };
        signal_button_release_event().connect(sigc::track_obj(cb, *this));
        AddPointerCursor(*this);
    }
    m_overlay.add(m_box);
    if (conn.IsVerified) {
        try {
            const static auto checkmarks_path = Abaddon::GetResPath("/checkmark.png");
            static auto pb = Gdk::Pixbuf::create_from_file(checkmarks_path, 24, 24);
            m_check = Gtk::manage(new Gtk::Image(pb));
            m_check->get_style_context()->add_class("profile-connection-check");
            m_check->set_margin_end(25);
            m_check->set_valign(Gtk::ALIGN_CENTER);
            m_check->set_halign(Gtk::ALIGN_END);
            m_check->show();
            m_overlay.add_overlay(*m_check);
        } catch (const Glib::Exception &e) {}
    }
    m_overlay.set_hexpand(false);
    m_overlay.set_halign(Gtk::ALIGN_START);
    add(m_overlay);
    show_all_children();
}

ConnectionsContainer::ConnectionsContainer() {
    get_style_context()->add_class("profile-connections");
    set_column_homogeneous(true);
    set_row_spacing(10);
    set_column_spacing(10);
    show_all_children();
}

void ConnectionsContainer::SetConnections(const std::vector<ConnectionData> &connections) {
    for (auto child : get_children())
        delete child;

    static const std::unordered_set<std::string> supported_services = {
        "battlenet",
        "ebay",
        "epicgames",
        "facebook",
        "github",
        "instagram",
        "leagueoflegends",
        "paypal",
        "playstation",
        "reddit",
        "riotgames",
        "skype",
        "spotify",
        "steam",
        "tiktok",
        "twitch",
        "twitter",
        "xbox",
        "youtube",
    };

    int i = 0;
    for (const auto &conn : connections) {
        if (supported_services.find(conn.Type) == supported_services.end()) continue;
        auto widget = Gtk::manage(new ConnectionItem(conn));
        widget->show();
        attach(*widget, i % 2, i / 2, 1, 1);
        i++;
    }

    set_halign(Gtk::ALIGN_FILL);
    set_hexpand(true);
}

NotesContainer::NotesContainer()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL) {
    get_style_context()->add_class("profile-notes");
    m_label.get_style_context()->add_class("profile-notes-label");
    m_note.get_style_context()->add_class("profile-notes-text");

    m_label.set_markup("<b>NOTE</b>");
    m_label.set_halign(Gtk::ALIGN_START);

    m_note.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    m_note.signal_key_press_event().connect(sigc::mem_fun(*this, &NotesContainer::OnNoteKeyPress), false);

    add(m_label);
    add(m_note);
    show_all_children();
}

void NotesContainer::SetNote(const std::string &note) {
    m_note.get_buffer()->set_text(note);
}

void NotesContainer::UpdateNote() {
    auto text = m_note.get_buffer()->get_text();
    if (text.size() > 256)
        text = text.substr(0, 256);
    m_signal_update_note.emit(text);
}

bool NotesContainer::OnNoteKeyPress(GdkEventKey *event) {
    if (event->type != GDK_KEY_PRESS) return false;
    const auto text = m_note.get_buffer()->get_text();
    if (event->keyval == GDK_KEY_Return) {
        if (event->state & GDK_SHIFT_MASK) {
            int newlines = 0;
            for (const auto c : text)
                if (c == '\n') newlines++;
            return newlines >= 5;
        } else {
            UpdateNote();
            return true;
        }
    }

    return false;
}

NotesContainer::type_signal_update_note NotesContainer::signal_update_note() {
    return m_signal_update_note;
}

BioContainer::BioContainer()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL) {
    m_label.set_markup("<b>ABOUT ME</b>");
    m_label.set_halign(Gtk::ALIGN_START);
    m_bio.set_halign(Gtk::ALIGN_START);
    m_bio.set_line_wrap(true);
    m_bio.set_line_wrap_mode(Pango::WRAP_WORD_CHAR);

    m_label.show();
    m_bio.show();

    add(m_label);
    add(m_bio);
}

void BioContainer::SetBio(const std::string &bio) {
    m_bio.set_text(bio);
}

ProfileUserInfoPane::ProfileUserInfoPane(Snowflake ID)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , UserID(ID) {
    get_style_context()->add_class("profile-info-pane");
    m_created.get_style_context()->add_class("profile-info-created");

    m_note.signal_update_note().connect([this](const Glib::ustring &note) {
        auto cb = [](DiscordError code) {
            if (code != DiscordError::NONE) {
                Gtk::MessageDialog dlg("Failed to set note", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
                dlg.set_position(Gtk::WIN_POS_CENTER);
                dlg.run();
            }
        };
        Abaddon::Get().GetDiscordClient().SetUserNote(UserID, note, sigc::track_obj(cb, *this));
    });

    auto &discord = Abaddon::Get().GetDiscordClient();
    auto note_update_cb = [this](Snowflake id, const std::string &note) {
        if (id == UserID)
            m_note.SetNote(note);
    };
    discord.signal_note_update().connect(sigc::track_obj(note_update_cb, m_note));

    auto fetch_note_cb = [this](const std::string &note) {
        m_note.SetNote(note);
    };
    discord.FetchUserNote(UserID, sigc::track_obj(fetch_note_cb, *this));

    m_created.set_halign(Gtk::ALIGN_START);
    m_created.set_margin_top(5);
    m_created.set_text("Account created: " + ID.GetLocalTimestamp());

    m_conns.set_halign(Gtk::ALIGN_START);
    m_conns.set_hexpand(true);

    m_created.show();
    m_note.show();
    m_conns.show();

    add(m_created);
    add(m_bio);
    add(m_note);
    add(m_conns);
}

void ProfileUserInfoPane::SetProfile(const UserProfileData &data) {
    if (data.User.Bio.has_value() && !data.User.Bio->empty()) {
        m_bio.SetBio(*data.User.Bio);
        m_bio.show();
    } else {
        m_bio.hide();
    }

    m_conns.SetConnections(data.ConnectedAccounts);
}
