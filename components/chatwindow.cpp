#include "chatwindow.hpp"
#include "chatmessage.hpp"
#include "../abaddon.hpp"

ChatWindow::ChatWindow() {
    m_set_messsages_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::SetMessagesInternal));
    m_new_message_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::AddNewMessageInternal));
    m_delete_message_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::DeleteMessageInternal));
    m_update_message_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::UpdateMessageInternal));
    m_history_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::AddNewHistoryInternal));

    m_main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    m_list = Gtk::manage(new Gtk::ListBox);
    m_scroll = Gtk::manage(new Gtk::ScrolledWindow);
    m_input = Gtk::manage(new Gtk::TextView);
    m_input_scroll = Gtk::manage(new Gtk::ScrolledWindow);

    m_main->get_style_context()->add_class("messages");
    m_list->get_style_context()->add_class("messages");
    m_input->get_style_context()->add_class("message-input");

    m_input->signal_key_press_event().connect(sigc::mem_fun(*this, &ChatWindow::on_key_press_event), false);

    m_main->set_hexpand(true);
    m_main->set_vexpand(true);

    m_scroll->signal_edge_reached().connect(sigc::mem_fun(*this, &ChatWindow::on_scroll_edge_overshot));

    auto v = m_scroll->get_vadjustment();
    v->signal_value_changed().connect([this, v] {
        m_should_scroll_to_bottom = v->get_upper() - v->get_page_size() <= v->get_value();
    });

    m_scroll->set_can_focus(false);
    m_scroll->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);

    m_list->signal_size_allocate().connect([this](Gtk::Allocation &) {
        if (m_should_scroll_to_bottom)
            ScrollToBottom();
    });

    m_list->set_selection_mode(Gtk::SELECTION_NONE);
    m_list->set_hexpand(true);
    m_list->set_vexpand(true);
    m_list->set_focus_hadjustment(m_scroll->get_hadjustment());
    m_list->set_focus_vadjustment(m_scroll->get_vadjustment());

    m_input->set_hexpand(false);
    m_input->set_halign(Gtk::ALIGN_FILL);
    m_input->set_wrap_mode(Gtk::WRAP_WORD_CHAR);

    m_input_scroll->set_max_content_height(200);
    m_input_scroll->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_input_scroll->add(*m_input);
    m_scroll->add(*m_list);
    m_main->add(*m_scroll);
    m_main->add(*m_input_scroll);
}

Gtk::Widget *ChatWindow::GetRoot() const {
    return m_main;
}

void ChatWindow::Clear() {
    m_update_mutex.lock();
    m_set_messages_queue.push(std::set<Snowflake>());
    m_set_messsages_dispatch.emit();
    m_update_mutex.unlock();
}

void ChatWindow::SetMessages(const std::set<Snowflake> &msgs) {
    m_update_mutex.lock();
    m_set_messages_queue.push(msgs);
    m_set_messsages_dispatch.emit();
    m_update_mutex.unlock();
}

void ChatWindow::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
}

void ChatWindow::AddNewMessage(Snowflake id) {
    m_update_mutex.lock();
    m_new_message_queue.push(id);
    m_new_message_dispatch.emit();
    m_update_mutex.unlock();
}

void ChatWindow::DeleteMessage(Snowflake id) {
    m_update_mutex.lock();
    m_delete_message_queue.push(id);
    m_delete_message_dispatch.emit();
    m_update_mutex.unlock();
}

void ChatWindow::UpdateMessage(Snowflake id) {
    m_update_mutex.lock();
    m_update_message_queue.push(id);
    m_update_message_dispatch.emit();
    m_update_mutex.unlock();
}

void ChatWindow::AddNewHistory(const std::vector<Snowflake> &id) {
    std::set<Snowflake> x;
    for (const auto &s : id)
        x.insert(s);
    m_update_mutex.lock();
    m_history_queue.push(x);
    m_history_dispatch.emit();
    m_update_mutex.unlock();
}

void ChatWindow::InsertChatInput(std::string text) {
    // shouldn't need a mutex cuz called from gui action
    m_input->get_buffer()->insert_at_cursor(text);
    m_input->grab_focus();
}

Snowflake ChatWindow::GetOldestListedMessage() {
    Snowflake m;

    for (const auto &[id, widget] : m_id_to_widget) {
        if (id < m)
            m = id;
    }

    return m;
}

Snowflake ChatWindow::GetActiveChannel() const {
    return m_active_channel;
}

bool ChatWindow::on_key_press_event(GdkEventKey *e) {
    if (e->keyval == GDK_KEY_Return) {
        if (e->state & GDK_SHIFT_MASK)
            return false;

        auto buf = m_input->get_buffer();
        auto text = buf->get_text();
        if (text.size() == 0) return true;
        buf->set_text("");

        m_signal_action_chat_submit.emit(text, m_active_channel);

        return true;
    }

    return false;
}

ChatMessageItemContainer *ChatWindow::CreateMessageComponent(Snowflake id) {
    auto *container = ChatMessageItemContainer::FromMessage(id);
    return container;
}

void ChatWindow::ProcessNewMessage(Snowflake id, bool prepend) {
    const auto &client = Abaddon::Get().GetDiscordClient();
    if (!client.IsStarted()) return; // e.g. load channel and then dc
    const auto *data = client.GetMessage(id);
    if (data == nullptr) return;

    ChatMessageHeader *last_row = nullptr;
    bool should_attach = false;
    if (m_num_rows > 0) {
        if (prepend)
            last_row = dynamic_cast<ChatMessageHeader *>(m_list->get_row_at_index(0));
        else
            last_row = dynamic_cast<ChatMessageHeader *>(m_list->get_row_at_index(m_num_rows - 1));

        if (last_row != nullptr)
            if (last_row->UserID == data->Author.ID)
                should_attach = true;
    }

    ChatMessageHeader *header;
    if (should_attach) {
        header = last_row;
    } else {
        const auto guild_id = client.GetChannel(m_active_channel)->GuildID;
        const auto user_id = data->Author.ID;
        const auto *user = client.GetUser(user_id);
        if (user == nullptr) return;

        header = Gtk::manage(new ChatMessageHeader(data));
        header->signal_action_insert_mention().connect([this, user_id]() {
            m_signal_action_insert_mention.emit(user_id);
        });

        header->signal_action_open_user_menu().connect([this, user_id, guild_id](const GdkEvent *event) {
            m_signal_action_open_user_menu.emit(event, user_id, guild_id);
        });

        m_num_rows++;
    }

    auto *content = CreateMessageComponent(id);
    if (content != nullptr) {
        header->AddContent(content, prepend);
        m_id_to_widget[id] = content;

        content->signal_action_delete().connect([this, id] {
            m_signal_action_message_delete.emit(m_active_channel, id);
        });
        content->signal_action_edit().connect([this, id] {
            m_signal_action_message_edit.emit(m_active_channel, id);
        });
        content->signal_image_load().connect([this, id](std::string url) {
            auto &mgr = Abaddon::Get().GetImageManager();
            mgr.LoadFromURL(url, [this, id, url](Glib::RefPtr<Gdk::Pixbuf> buf) {
                if (m_id_to_widget.find(id) != m_id_to_widget.end()) {
                    auto *x = dynamic_cast<ChatMessageItemContainer *>(m_id_to_widget.at(id));
                    if (x != nullptr)
                        x->UpdateImage(url, buf);
                }
            });
        });
        content->signal_action_channel_click().connect([this](const Snowflake &id) {
            m_signal_action_channel_click.emit(id);
        });
    }

    header->set_margin_left(5);
    header->show_all();

    if (!should_attach) {
        if (prepend)
            m_list->prepend(*header);
        else
            m_list->add(*header);
    }
}

void ChatWindow::SetMessagesInternal() {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    const auto *msgs = &m_set_messages_queue.front();

    // empty the listbox
    auto children = m_list->get_children();
    auto it = children.begin();
    while (it != children.end()) {
        delete *it;
        it++;
    }

    m_num_rows = 0;
    m_id_to_widget.clear();

    for (const auto &id : *msgs) {
        ProcessNewMessage(id, false);
    }

    m_set_messages_queue.pop();
}

void ChatWindow::AddNewMessageInternal() {
    m_update_mutex.lock();
    const auto id = m_new_message_queue.front();
    m_new_message_queue.pop();
    m_update_mutex.unlock();
    ProcessNewMessage(id, false);
}

void ChatWindow::DeleteMessageInternal() {
    m_update_mutex.lock();
    const auto id = m_delete_message_queue.front();
    m_delete_message_queue.pop();
    m_update_mutex.unlock();

    auto widget = m_id_to_widget.find(id);
    if (widget == m_id_to_widget.end()) return;

    auto *x = dynamic_cast<ChatMessageItemContainer *>(widget->second);
    if (x != nullptr)
        x->UpdateAttributes();
}

void ChatWindow::UpdateMessageInternal() {
    m_update_mutex.lock();
    const auto id = m_update_message_queue.front();
    m_update_message_queue.pop();
    m_update_mutex.unlock();

    auto widget = m_id_to_widget.find(id);
    if (widget == m_id_to_widget.end()) return;

    auto *x = dynamic_cast<ChatMessageItemContainer *>(widget->second);
    if (x != nullptr) {
        x->UpdateContent();
        x->UpdateAttributes();
    }
}

void ChatWindow::AddNewHistoryInternal() {
    m_update_mutex.lock();
    auto msgs = m_history_queue.front();
    m_history_queue.pop();
    m_update_mutex.unlock();

    for (auto it = msgs.rbegin(); it != msgs.rend(); it++)
        ProcessNewMessage(*it, true);
}

void ChatWindow::on_scroll_edge_overshot(Gtk::PositionType pos) {
    if (pos == Gtk::POS_TOP)
        m_signal_action_chat_load_history.emit(m_active_channel);
}

void ChatWindow::ScrollToBottom() {
    auto x = m_scroll->get_vadjustment();
    x->set_value(x->get_upper());
}

ChatWindow::type_signal_action_message_delete ChatWindow::signal_action_message_delete() {
    return m_signal_action_message_delete;
}

ChatWindow::type_signal_action_message_edit ChatWindow::signal_action_message_edit() {
    return m_signal_action_message_edit;
}

ChatWindow::type_signal_action_chat_submit ChatWindow::signal_action_chat_submit() {
    return m_signal_action_chat_submit;
}

ChatWindow::type_signal_action_chat_load_history ChatWindow::signal_action_chat_load_history() {
    return m_signal_action_chat_load_history;
}

ChatWindow::type_signal_action_channel_click ChatWindow::signal_action_channel_click() {
    return m_signal_action_channel_click;
}

ChatWindow::type_signal_action_insert_mention ChatWindow::signal_action_insert_mention() {
    return m_signal_action_insert_mention;
}

ChatWindow::type_signal_action_open_user_menu ChatWindow::signal_action_open_user_menu() {
    return m_signal_action_open_user_menu;
}
