#include "chatwindow.hpp"
#include "../abaddon.hpp"
#include <map>

ChatWindow::ChatWindow() {
    m_message_set_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::SetMessagesInternal));
    m_new_message_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::AddNewMessageInternal));
    m_new_history_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::AddNewHistoryInternal));
    m_message_delete_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::DeleteMessageInternal));

    m_main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    m_listbox = Gtk::manage(new Gtk::ListBox);
    m_viewport = Gtk::manage(new Gtk::Viewport(Gtk::Adjustment::create(0, 0, 0, 0, 0, 0), Gtk::Adjustment::create(0, 0, 0, 0, 0, 0)));
    m_scroll = Gtk::manage(new Gtk::ScrolledWindow);
    m_input = Gtk::manage(new Gtk::TextView);
    m_entry_scroll = Gtk::manage(new Gtk::ScrolledWindow);

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

void ChatWindow::SetAbaddon(Abaddon *ptr) {
    m_abaddon = ptr;
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

ChatDisplayType ChatWindow::GetMessageDisplayType(const MessageData *data) {
    if (data->Type == MessageType::DEFAULT && data->Content.size() > 0)
        return ChatDisplayType::Text;

    return ChatDisplayType::Unknown;
}

ChatMessageItem *ChatWindow::CreateChatEntryComponentText(const MessageData *data) {
    return Gtk::manage(new ChatMessageTextItem(data));
}

ChatMessageItem *ChatWindow::CreateChatEntryComponent(const MessageData *data) {
    ChatMessageItem *item = nullptr;
    switch (GetMessageDisplayType(data)) {
        case ChatDisplayType::Text:
            item = CreateChatEntryComponentText(data);
            break;
    }

    if (item != nullptr)
        item->ID = data->ID;

    return item;
}

void ChatWindow::ProcessMessage(const MessageData *data, bool prepend) {
    auto create_new_row = [&]() {
        auto *item = CreateChatEntryComponent(data);
        if (item != nullptr) {
            if (prepend)
                m_listbox->prepend(*item);
            else
                m_listbox->add(*item);
            m_num_rows++;
        }
    };

    // if the last row's message's author is the same as the new one's, then append the new message content to the last row
    if (m_num_rows > 0) {
        ChatMessageItem *item;
        if (prepend)
            item = dynamic_cast<ChatMessageItem *>(m_listbox->get_row_at_index(0));
        else
            item = dynamic_cast<ChatMessageItem *>(m_listbox->get_row_at_index(m_num_rows - 1));
        assert(item != nullptr);
        auto *previous_data = m_abaddon->GetDiscordClient().GetMessage(item->ID);

        auto new_type = GetMessageDisplayType(data);
        auto old_type = GetMessageDisplayType(previous_data);

        if ((data->Author.ID == previous_data->Author.ID) && (new_type == old_type && new_type == ChatDisplayType::Text)) {
            auto *text_item = dynamic_cast<ChatMessageTextItem *>(item);
            if (prepend)
                text_item->PrependNewContent(data->Content);
            else
                text_item->AppendNewContent(data->Content);
        } else {
            create_new_row();
        }
    } else {
        create_new_row();
    }
}

bool ChatWindow::on_key_press_event(GdkEventKey *e) {
    if (e->keyval == GDK_KEY_Return) {
        auto buffer = m_input->get_buffer();

        if (e->state & GDK_SHIFT_MASK)
            return false;

        auto text = buffer->get_text();
        buffer->set_text("");

        m_abaddon->ActionChatInputSubmit(text, m_active_channel);

        return true;
    }

    return false;
}

void ChatWindow::on_scroll_edge_overshot(Gtk::PositionType pos) {
    if (pos == Gtk::POS_TOP)
        m_abaddon->ActionChatLoadHistory(m_active_channel);
}

void ChatWindow::SetMessages(std::unordered_set<const MessageData *> msgs) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_message_set_queue.push(msgs);
    m_message_set_dispatch.emit();
}

void ChatWindow::AddNewMessage(Snowflake id) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_new_message_queue.push(id);
    m_new_message_dispatch.emit();
}

void ChatWindow::AddNewHistory(const std::vector<MessageData> &msgs) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    std::vector<Snowflake> x;
    for (const auto &msg : msgs)
        x.push_back(msg.ID);
    m_new_history_queue.push(x);
    m_new_history_dispatch.emit();
}

void ChatWindow::DeleteMessage(Snowflake id) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_message_delete_queue.push(id);
    m_message_delete_dispatch.emit();
}

void ChatWindow::ClearMessages() {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_message_set_queue.push(std::unordered_set<const MessageData *>());
    m_message_set_dispatch.emit();
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

    auto data = m_abaddon->GetDiscordClient().GetMessage(id);
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
        ProcessMessage(m_abaddon->GetDiscordClient().GetMessage(*it), true);
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

    ChatMessageItem *row = nullptr;
    for (const auto &child : m_listbox->get_children()) {
        ChatMessageItem *tmp = dynamic_cast<ChatMessageItem *>(child);
        if (tmp == nullptr) continue;
        if (tmp->ID == id) {
            row = tmp;
            break;
        }
    }

    if (row == nullptr) return;

    // todo actually delete it when it becomes setting

    row->MarkAsDeleted();
}

void ChatWindow::SetMessagesInternal() {
    auto children = m_listbox->get_children();
    auto it = children.begin();

    while (it != children.end()) {
        delete *it;
        it++;
    }

    m_num_rows = 0;

    std::unordered_set<const MessageData *> *msgs;
    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        msgs = &m_message_set_queue.front();
    }

    // sort
    std::map<Snowflake, const MessageData *> sorted_messages;
    for (const auto msg : *msgs)
        sorted_messages[msg->ID] = msg;

    for (const auto &[id, msg] : sorted_messages) {
        ProcessMessage(msg);
    }

    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        m_message_set_queue.pop();
    }
}
