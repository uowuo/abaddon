#include "userinfopane.hpp"
#include <unordered_set>
#include "../../abaddon.hpp"

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
        "github",
        "leagueoflegends",
        "reddit",
        "skype",
        "spotify",
        "steam",
        "twitch",
        "twitter",
        "xbox",
        "youtube",
        "facebook"
    };

    for (int i = 0; i < connections.size(); i++) {
        const auto &conn = connections[i];
        if (supported_services.find(conn.Type) == supported_services.end()) continue;
        Glib::RefPtr<Gdk::Pixbuf> pixbuf;
        try {
            pixbuf = Gdk::Pixbuf::create_from_file("./res/" + conn.Type + ".png", 32, 32);
        } catch (const Glib::Exception &e) {}
        std::string url;
        if (conn.Type == "github")
            url = "https://github.com/" + conn.Name;
        else if (conn.Type == "steam")
            url = "https://steamcommunity.com/profiles/" + conn.ID;
        else if (conn.Type == "twitch")
            url = "https://twitch.tv/" + conn.Name;
        else if (conn.Type == "twitter")
            url = "https://twitter.com/i/user/" + conn.ID;
        else if (conn.Type == "spotify")
            url = "https://open.spotify.com/user/" + conn.ID;
        else if (conn.Type == "reddit")
            url = "https://reddit.com/u/" + conn.Name;
        else if (conn.Type == "youtube")
            url = "https://www.youtube.com/channel/" + conn.ID;
        else if (conn.Type == "facebook")
            url = "https://www.facebook.com/" + conn.ID;
        auto *ev = Gtk::manage(new Gtk::EventBox);
        auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        if (pixbuf) {
            auto *img = Gtk::manage(new Gtk::Image(pixbuf));
            img->get_style_context()->add_class("profile-connection-image");
            box->add(*img);
        }
        auto *lbl = Gtk::manage(new Gtk::Label(conn.Name));
        box->set_halign(Gtk::ALIGN_START);
        box->set_size_request(200, -1);
        box->get_style_context()->add_class("profile-connection");
        lbl->get_style_context()->add_class("profile-connection-label");
        lbl->set_valign(Gtk::ALIGN_CENTER);
        lbl->set_single_line_mode(true);
        lbl->set_ellipsize(Pango::ELLIPSIZE_END);
        box->add(*lbl);
        if (url != "") {
            auto cb = [this, url](GdkEventButton *event) -> bool {
                if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
                    LaunchBrowser(url);
                    return true;
                }
                return false;
            };
            ev->signal_button_press_event().connect(sigc::track_obj(cb, *ev));
            AddPointerCursor(*ev);
        }
        ev->add(*box);
        ev->show_all();
        attach(*ev, i % 2, i / 2, 1, 1);
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

ProfileUserInfoPane::ProfileUserInfoPane(Snowflake ID)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , UserID(ID) {
    get_style_context()->add_class("profile-info-pane");

    m_note.signal_update_note().connect([this](const Glib::ustring &note) {
        auto cb = [this](bool success) {
            if (!success) {
                Gtk::MessageDialog dlg("Failed to set note", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
                dlg.set_position(Gtk::WIN_POS_CENTER);
                dlg.run();
            }
        };
        Abaddon::Get().GetDiscordClient().SetUserNote(UserID, note, sigc::track_obj(cb, *this));
    });

    auto &discord = Abaddon::Get().GetDiscordClient();
    discord.signal_note_update().connect([this](Snowflake id, std::string note) {
        if (id == UserID)
            m_note.SetNote(note);
    });

    auto fetch_note_cb = [this](const std::string &note) {
        m_note.SetNote(note);
    };
    discord.FetchUserNote(UserID, sigc::track_obj(fetch_note_cb, *this));

    m_conns.set_halign(Gtk::ALIGN_START);
    m_conns.set_hexpand(true);

    add(m_note);
    add(m_conns);
    show_all_children();
}

void ProfileUserInfoPane::SetConnections(const std::vector<ConnectionData> &connections) {
    m_conns.SetConnections(connections);
}
