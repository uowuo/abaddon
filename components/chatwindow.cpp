#include "chatwindow.hpp"
#include "../abaddon.hpp"
#include <map>

ChatWindow::ChatWindow() {
    m_message_set_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::SetMessagesInternal));
    m_new_message_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::AddNewMessageInternal));
    m_new_history_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::AddNewHistoryInternal));
    m_message_delete_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::DeleteMessageInternal));
    m_message_edit_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::UpdateMessageInternal));

    m_main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    m_listbox = Gtk::manage(new Gtk::ListBox);
    m_viewport = Gtk::manage(new Gtk::Viewport(Gtk::Adjustment::create(0, 0, 0, 0, 0, 0), Gtk::Adjustment::create(0, 0, 0, 0, 0, 0)));
    m_scroll = Gtk::manage(new Gtk::ScrolledWindow);
    m_input = Gtk::manage(new Gtk::TextView);
    m_entry_scroll = Gtk::manage(new Gtk::ScrolledWindow);

    m_main->get_style_context()->add_class("messages");
    m_listbox->get_style_context()->add_class("messages"); // maybe hacky
    m_input->get_style_context()->add_class("message-input");

    m_input->signal_key_press_event().connect(sigc::mem_fun(*this, &ChatWindow::on_key_press_event), false);

    m_main->set_hexpand(true);
    m_main->set_vexpand(true);
    m_main->show();

    m_scroll->signal_edge_reached().connect(sigc::mem_fun(*this, &ChatWindow::on_scroll_edge_overshot));

    auto vadj = m_scroll->get_vadjustment();
    vadj->signal_value_changed().connect([&, vadj]() {
        m_scroll_to_bottom = vadj->get_upper() - vadj->get_page_size() <= vadj->get_value();
    });

    m_scroll->set_can_focus(false);
    m_scroll->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
    m_scroll->show();

    m_listbox->signal_size_allocate().connect([this](Gtk::Allocation &) {
        if (m_scroll_to_bottom)
            ScrollToBottom();
    });

    m_listbox->set_selection_mode(Gtk::SELECTION_NONE);
    m_listbox->set_hexpand(true);
    m_listbox->set_vexpand(true);
    m_listbox->set_focus_hadjustment(m_scroll->get_hadjustment());
    m_listbox->set_focus_vadjustment(m_scroll->get_vadjustment());
    m_listbox->show();

    m_viewport->set_can_focus(false);
    m_viewport->set_valign(Gtk::ALIGN_END);
    m_viewport->set_vscroll_policy(Gtk::SCROLL_NATURAL);
    m_viewport->set_shadow_type(Gtk::SHADOW_NONE);
    m_viewport->show();

    m_input->set_hexpand(true);
    m_input->set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    m_input->show();

    m_entry_scroll->set_max_content_height(150);
    m_entry_scroll->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    m_entry_scroll->add(*m_input);
    m_entry_scroll->show();

    m_viewport->add(*m_listbox);
    m_scroll->add(*m_viewport);
    m_main->add(*m_scroll);
    m_main->add(*m_entry_scroll);
}

Gtk::Widget *ChatWindow::GetRoot() const {
    return m_main;
}

void ChatWindow::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
}

Snowflake ChatWindow::GetActiveChannel() const {
    return m_active_channel;
}

ChatDisplayType ChatWindow::GetMessageDisplayType(const Message *data) {
    if (data->Type == MessageType::DEFAULT && data->Content.size() > 0)
        return ChatDisplayType::Text;
    else if (data->Type == MessageType::DEFAULT && data->Embeds.size() > 0)
        return ChatDisplayType::Embed;

    return ChatDisplayType::Unknown;
}

ChatMessageItem *ChatWindow::CreateMessageComponent(const Message *data) {
    auto type = GetMessageDisplayType(data);
    ChatMessageItem *widget = nullptr;

    if (type == ChatDisplayType::Text) {
        widget = Gtk::manage(new ChatMessageTextItem(data));

        widget->signal_action_message_delete().connect([this](Snowflake channel_id, Snowflake id) {
            m_signal_action_message_delete.emit(channel_id, id);
        });
        widget->signal_action_message_edit().connect([this](Snowflake channel_id, Snowflake id) {
            m_signal_action_message_edit.emit(channel_id, id);
        });
    } else if (type == ChatDisplayType::Embed) {
        widget = Gtk::manage(new ChatMessageEmbedItem(data));
    }

    if (widget == nullptr) return nullptr;
    widget->ChannelID = m_active_channel;
    m_id_to_widget[data->ID] = widget;
    return widget;
}

void ChatWindow::ProcessMessage(const Message *data, bool prepend) {
    if (!Abaddon::Get().GetDiscordClient().IsStarted()) return;

    ChatMessageContainer *last_row = nullptr;
    bool should_attach = false;
    if (m_num_rows > 0) {
        if (prepend)
            last_row = dynamic_cast<ChatMessageContainer *>(m_listbox->get_row_at_index(0));
        else
            last_row = dynamic_cast<ChatMessageContainer *>(m_listbox->get_row_at_index(m_num_rows - 1));
        if (last_row != nullptr) { // this should always be true tbh
            if (last_row->UserID == data->Author.ID)
                should_attach = true;
        }
    }

    ChatMessageContainer *container;
    if (should_attach) {
        container = last_row;
    } else {
        container = Gtk::manage(new ChatMessageContainer(data)); // only accesses timestamp and user
        container->Update();

        auto user_id = data->Author.ID;
        const auto *user = Abaddon::Get().GetDiscordClient().GetUser(user_id);
        if (user == nullptr) return;
        Abaddon::Get().GetImageManager().LoadFromURL(user->GetAvatarURL(), [this, user_id](Glib::RefPtr<Gdk::Pixbuf> buf) {
            // am i retarded?
            Glib::signal_idle().connect([this, buf, user_id]() -> bool {
                auto children = m_listbox->get_children();
                for (auto child : children) {
                    auto *row = dynamic_cast<ChatMessageContainer *>(child);
                    if (row == nullptr) continue;
                    if (row->UserID == user_id) {
                        row->SetAvatarFromPixbuf(buf);
                    }
                }

                return false;
            });
        });
        m_num_rows++;
    }

    ChatMessageItem *widget = CreateMessageComponent(data);

    if (widget != nullptr) {
        widget->SetContainer(container);
        container->AddNewContent(dynamic_cast<Gtk::Widget *>(widget), prepend);
    }

    container->set_margin_left(5);
    container->show_all();

    if (!should_attach) {
        if (prepend)
            m_listbox->prepend(*container);
        else
            m_listbox->add(*container);
    }
}

bool ChatWindow::on_key_press_event(GdkEventKey *e) {
    if (e->keyval == GDK_KEY_Return) {
        auto buffer = m_input->get_buffer();

        if (e->state & GDK_SHIFT_MASK)
            return false;

        auto text = buffer->get_text();
        buffer->set_text("");

        m_signal_action_chat_submit.emit(text, m_active_channel);

        return true;
    }

    return false;
}

void ChatWindow::on_scroll_edge_overshot(Gtk::PositionType pos) {
    if (pos == Gtk::POS_TOP)
        m_signal_action_chat_load_history.emit(m_active_channel);
}

void ChatWindow::SetMessages(std::set<Snowflake> msgs) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_message_set_queue.push(msgs);
    m_message_set_dispatch.emit();
}

void ChatWindow::AddNewMessage(Snowflake id) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_new_message_queue.push(id);
    m_new_message_dispatch.emit();
}

void ChatWindow::AddNewHistory(const std::vector<Snowflake> &msgs) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_new_history_queue.push(msgs);
    m_new_history_dispatch.emit();
}

void ChatWindow::DeleteMessage(Snowflake id) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_message_delete_queue.push(id);
    m_message_delete_dispatch.emit();
}

void ChatWindow::UpdateMessage(Snowflake id) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_message_edit_queue.push(id);
    m_message_edit_dispatch.emit();
}

void ChatWindow::Clear() {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_active_channel = Snowflake::Invalid;
    m_message_set_queue.push(std::set<Snowflake>());
    m_message_set_dispatch.emit();
}

void ChatWindow::InsertChatInput(std::string text) {
    auto buf = m_input->get_buffer();
    buf->insert_at_cursor(text);
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

void ChatWindow::ScrollToBottom() {
    auto x = m_scroll->get_vadjustment();
    x->set_value(x->get_upper());
}

void ChatWindow::AddNewMessageInternal() {
    Snowflake id;
    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        id = m_new_message_queue.front();
        m_new_message_queue.pop();
    }

    auto data = Abaddon::Get().GetDiscordClient().GetMessage(id);
    ProcessMessage(data);
}

// todo this keeps the scrollbar at the top
void ChatWindow::AddNewHistoryInternal() {
    std::set<Snowflake> msgs;
    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        auto vec = m_new_history_queue.front();
        msgs = std::set<Snowflake>(vec.begin(), vec.end());
    }

    for (auto it = msgs.rbegin(); it != msgs.rend(); it++) {
        ProcessMessage(Abaddon::Get().GetDiscordClient().GetMessage(*it), true);
    }

    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        m_new_history_queue.pop();
    }
}

void ChatWindow::DeleteMessageInternal() {
    Snowflake id;
    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        id = m_message_delete_queue.front();
        m_message_delete_queue.pop();
    }

    if (m_id_to_widget.find(id) == m_id_to_widget.end())
        return;

    // todo actually delete it when it becomes setting

    auto *item = m_id_to_widget.at(id);
    item->Update();
}

void ChatWindow::UpdateMessageInternal() {
    Snowflake id;
    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        id = m_message_edit_queue.front();
        m_message_edit_queue.pop();
    }

    if (m_id_to_widget.find(id) == m_id_to_widget.end())
        return;

    // GetMessage should give us the new object at this point
    auto *msg = Abaddon::Get().GetDiscordClient().GetMessage(id);
    auto *item = dynamic_cast<Gtk::Widget *>(m_id_to_widget.at(id));
    if (item != nullptr) {
        ChatMessageContainer *container = dynamic_cast<ChatMessageItem *>(item)->GetContainer();
        int idx = container->RemoveItem(item);
        if (idx == -1) return;
        auto *new_widget = CreateMessageComponent(msg);
        new_widget->SetContainer(container);
        container->AddNewContentAtIndex(dynamic_cast<Gtk::Widget *>(new_widget), idx);
    }
}

void ChatWindow::SetMessagesInternal() {
    auto children = m_listbox->get_children();
    auto it = children.begin();

    while (it != children.end()) {
        delete *it;
        it++;
    }

    m_num_rows = 0;
    m_id_to_widget.clear();

    std::set<Snowflake> *msgs;
    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        msgs = &m_message_set_queue.front();
    }

    // sort
    std::map<Snowflake, const Message *> sorted_messages;
    for (const auto id : *msgs)
        sorted_messages[id] = Abaddon::Get().GetDiscordClient().GetMessage(id);

    for (const auto &[id, msg] : sorted_messages) {
        ProcessMessage(msg);
    }

    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        m_message_set_queue.pop();
    }
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
