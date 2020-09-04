#include "chatwindow.hpp"
#include "../abaddon.hpp"
#include <map>

ChatWindow::ChatWindow() {
    m_message_set_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::SetMessagesInternal));
    m_new_message_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::AddNewMessageInternal));
    m_new_history_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::AddNewHistoryInternal));
    m_message_delete_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::DeleteMessageInternal));
    m_message_edit_dispatch.connect(sigc::mem_fun(*this, &ChatWindow::UpdateMessageContentInternal));

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
    else if (data->Type == MessageType::DEFAULT && data->Embeds.size() > 0)
        return ChatDisplayType::Embed;

    return ChatDisplayType::Unknown;
}

void ChatWindow::ProcessMessage(const MessageData *data, bool prepend) {
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

    auto type = GetMessageDisplayType(data);

    ChatMessageContainer *container;
    if (should_attach) {
        container = last_row;
    } else {
        container = Gtk::manage(new ChatMessageContainer(data)); // only accesses timestamp and user
        m_num_rows++;
    }

    // actual content
    if (type == ChatDisplayType::Text) {
        auto *text = Gtk::manage(new ChatMessageTextItem(data));
        text->ID = data->ID;
        text->ChannelID = m_active_channel;
        text->SetAbaddon(m_abaddon);
        container->AddNewContent(text, prepend);
        m_id_to_widget[data->ID] = text;
    } else if (type == ChatDisplayType::Embed) {
        auto *widget = Gtk::manage(new ChatMessageEmbedItem(data));
        widget->ID = data->ID;
        widget->ChannelID = m_active_channel;
        widget->SetAbaddon(m_abaddon);
        container->AddNewContent(widget, prepend);
        m_id_to_widget[data->ID] = widget;
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

        m_abaddon->ActionChatInputSubmit(text, m_active_channel);

        return true;
    }

    return false;
}

void ChatWindow::on_scroll_edge_overshot(Gtk::PositionType pos) {
    if (pos == Gtk::POS_TOP)
        m_abaddon->ActionChatLoadHistory(m_active_channel);
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

void ChatWindow::UpdateMessageContent(Snowflake id) {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_message_edit_queue.push(id);
    m_message_edit_dispatch.emit();
}

void ChatWindow::ClearMessages() {
    std::scoped_lock<std::mutex> guard(m_update_mutex);
    m_message_set_queue.push(std::set<Snowflake>());
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

    if (m_id_to_widget.find(id) == m_id_to_widget.end())
        return;

    // todo actually delete it when it becomes setting

    auto *item = m_id_to_widget.at(id);
    item->MarkAsDeleted();
}

void ChatWindow::UpdateMessageContentInternal() {
    Snowflake id;
    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        id = m_message_edit_queue.front();
        m_message_edit_queue.pop();
    }

    if (m_id_to_widget.find(id) == m_id_to_widget.end())
        return;

    auto *msg = m_abaddon->GetDiscordClient().GetMessage(id);
    auto *item = dynamic_cast<ChatMessageTextItem *>(m_id_to_widget.at(id));
    if (item != nullptr) {
        item->EditContent(msg->Content);
        item->MarkAsEdited();
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
    std::map<Snowflake, const MessageData *> sorted_messages;
    for (const auto id : *msgs)
        sorted_messages[id] = m_abaddon->GetDiscordClient().GetMessage(id);

    for (const auto &[id, msg] : sorted_messages) {
        ProcessMessage(msg);
    }

    {
        std::scoped_lock<std::mutex> guard(m_update_mutex);
        m_message_set_queue.pop();
    }
}
