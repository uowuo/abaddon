#include "chatinput.hpp"

ChatInput::ChatInput() {
    get_style_context()->add_class("message-input");
    set_propagate_natural_height(true);
    set_min_content_height(20);
    set_max_content_height(250);
    set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    // hack
    auto cb = [this](GdkEventKey *e) -> bool {
        return event(reinterpret_cast<GdkEvent *>(e));
    };
    m_textview.signal_key_press_event().connect(cb, false);
    m_textview.set_hexpand(false);
    m_textview.set_halign(Gtk::ALIGN_FILL);
    m_textview.set_valign(Gtk::ALIGN_CENTER);
    m_textview.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    m_textview.show();
    add(m_textview);
}

void ChatInput::InsertText(const Glib::ustring &text) {
    GetBuffer()->insert_at_cursor(text);
    m_textview.grab_focus();
}

Glib::RefPtr<Gtk::TextBuffer> ChatInput::GetBuffer() {
    return m_textview.get_buffer();
}

// this isnt connected directly so that the chat window can handle stuff like the completer first
bool ChatInput::ProcessKeyPress(GdkEventKey *event) {
    if (event->keyval == GDK_KEY_Return) {
        if (event->state & GDK_SHIFT_MASK)
            return false;

        auto buf = GetBuffer();
        auto text = buf->get_text();
        // sometimes a message thats just newlines can sneak in if you hold down enter
        if (text.size() > 0 && !std::all_of(text.begin(), text.end(), [](gunichar c) -> bool { return c == gunichar('\n'); })) {
            buf->set_text("");
            m_signal_submit.emit(text);
            return true;
        }
    }

    return false;
}

ChatInput::type_signal_submit ChatInput::signal_submit() {
    return m_signal_submit;
}
