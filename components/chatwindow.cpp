#include "chatwindow.hpp"
#include "chatmessage.hpp"
#include "../abaddon.hpp"
#include "typingindicator.hpp"
#include "chatinput.hpp"

ChatWindow::ChatWindow() {
    m_main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    m_list = Gtk::manage(new Gtk::ListBox);
    m_scroll = Gtk::manage(new Gtk::ScrolledWindow);
    m_input = Gtk::manage(new ChatInput);
    m_typing_indicator = Gtk::manage(new TypingIndicator);

    m_typing_indicator->set_valign(Gtk::ALIGN_END);
    m_typing_indicator->show();

    m_main->get_style_context()->add_class("messages");
    m_list->get_style_context()->add_class("messages");

    m_main->set_hexpand(true);
    m_main->set_vexpand(true);

    m_scroll->signal_edge_reached().connect(sigc::mem_fun(*this, &ChatWindow::OnScrollEdgeOvershot));

    auto v = m_scroll->get_vadjustment();
    v->signal_value_changed().connect([this, v] {
        m_should_scroll_to_bottom = v->get_upper() - v->get_page_size() <= v->get_value();
    });

    m_scroll->set_can_focus(false);
    m_scroll->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
    m_scroll->show();

    m_list->signal_size_allocate().connect([this](Gtk::Allocation &) {
        if (m_should_scroll_to_bottom)
            ScrollToBottom();
    });

    m_list->set_selection_mode(Gtk::SELECTION_NONE);
    m_list->set_hexpand(true);
    m_list->set_vexpand(true);
    m_list->set_focus_hadjustment(m_scroll->get_hadjustment());
    m_list->set_focus_vadjustment(m_scroll->get_vadjustment());
    m_list->show();

    m_input->signal_submit().connect([this](const Glib::ustring &text) {
        if (m_active_channel.IsValid())
            m_signal_action_chat_submit.emit(text, m_active_channel);
    });
    m_input->signal_key_press_event().connect(sigc::mem_fun(*this, &ChatWindow::OnKeyPressEvent), false);
    m_input->show();

    m_completer.SetBuffer(m_input->GetBuffer());
    m_completer.SetGetChannelID([this]() -> auto {
        return m_active_channel;
    });

    m_completer.SetGetRecentAuthors([this]() -> auto {
        const auto &discord = Abaddon::Get().GetDiscordClient();
        std::vector<Snowflake> ret;

        std::map<Snowflake, Gtk::Widget *> ordered(m_id_to_widget.begin(), m_id_to_widget.end());

        for (auto it = ordered.crbegin(); it != ordered.crend(); it++) {
            const auto *widget = dynamic_cast<ChatMessageItemContainer *>(it->second);
            if (widget == nullptr) continue;
            const auto msg = discord.GetMessage(widget->ID);
            if (!msg.has_value()) continue;
            if (std::find(ret.begin(), ret.end(), msg->Author.ID) == ret.end())
                ret.push_back(msg->Author.ID);
        }

        const auto chan = discord.GetChannel(m_active_channel);
        if (chan->GuildID.has_value()) {
            const auto others = discord.GetUsersInGuild(*chan->GuildID);
            for (const auto id : others)
                if (std::find(ret.begin(), ret.end(), id) == ret.end())
                    ret.push_back(id);
        }

        return ret;
    });

    m_completer.show();

    m_scroll->add(*m_list);
    m_main->add(*m_scroll);
    m_main->add(m_completer);
    m_main->add(*m_input);
    m_main->add(*m_typing_indicator);
    m_main->show();
}

Gtk::Widget *ChatWindow::GetRoot() const {
    return m_main;
}

void ChatWindow::Clear() {
    SetMessages(std::set<Snowflake>());
}

void ChatWindow::SetMessages(const std::set<Snowflake> &msgs) {
    // empty the listbox
    auto children = m_list->get_children();
    auto it = children.begin();
    while (it != children.end()) {
        delete *it;
        it++;
    }

    m_num_rows = 0;
    m_id_to_widget.clear();

    for (const auto &id : msgs) {
        ProcessNewMessage(id, false);
    }
}

void ChatWindow::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
    m_typing_indicator->SetActiveChannel(id);
}

void ChatWindow::AddNewMessage(Snowflake id) {
    ProcessNewMessage(id, false);
}

void ChatWindow::DeleteMessage(Snowflake id) {
    auto widget = m_id_to_widget.find(id);
    if (widget == m_id_to_widget.end()) return;

    auto *x = dynamic_cast<ChatMessageItemContainer *>(widget->second);
    if (x != nullptr)
        x->UpdateAttributes();
}

void ChatWindow::UpdateMessage(Snowflake id) {
    auto widget = m_id_to_widget.find(id);
    if (widget == m_id_to_widget.end()) return;

    auto *x = dynamic_cast<ChatMessageItemContainer *>(widget->second);
    if (x != nullptr) {
        x->UpdateContent();
        x->UpdateAttributes();
    }
}

void ChatWindow::AddNewHistory(const std::vector<Snowflake> &id) {
    std::set<Snowflake> ids(id.begin(), id.end());
    for (auto it = ids.rbegin(); it != ids.rend(); it++)
        ProcessNewMessage(*it, true);
}

void ChatWindow::InsertChatInput(std::string text) {
    m_input->InsertText(text);
}

Snowflake ChatWindow::GetOldestListedMessage() {
    Snowflake m;

    for (const auto &[id, widget] : m_id_to_widget) {
        if (id < m)
            m = id;
    }

    return m;
}

void ChatWindow::UpdateReactions(Snowflake id) {
    auto it = m_id_to_widget.find(id);
    if (it == m_id_to_widget.end()) return;
    auto *widget = dynamic_cast<ChatMessageItemContainer *>(it->second);
    if (widget == nullptr) return;
    widget->UpdateReactions();
}

Snowflake ChatWindow::GetActiveChannel() const {
    return m_active_channel;
}

bool ChatWindow::OnKeyPressEvent(GdkEventKey *e) {
    if (m_completer.ProcessKeyPress(e))
        return true;

    if (m_input->ProcessKeyPress(e))
        return true;

    return false;
}

ChatMessageItemContainer *ChatWindow::CreateMessageComponent(Snowflake id) {
    auto *container = ChatMessageItemContainer::FromMessage(id);
    return container;
}

void ChatWindow::ProcessNewMessage(Snowflake id, bool prepend) {
    const auto &client = Abaddon::Get().GetDiscordClient();
    if (!client.IsStarted()) return; // e.g. load channel and then dc
    const auto data = client.GetMessage(id);
    if (!data.has_value()) return;

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
        const auto guild_id = *client.GetChannel(m_active_channel)->GuildID;
        const auto user_id = data->Author.ID;
        const auto user = client.GetUser(user_id);
        if (!user.has_value()) return;

        header = Gtk::manage(new ChatMessageHeader(&*data));
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
        content->signal_action_reaction_add().connect([this, id](const Glib::ustring &param) {
            m_signal_action_reaction_add.emit(id, param);
        });
        content->signal_action_reaction_remove().connect([this, id](const Glib::ustring &param) {
            m_signal_action_reaction_remove.emit(id, param);
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

void ChatWindow::OnScrollEdgeOvershot(Gtk::PositionType pos) {
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

ChatWindow::type_signal_action_reaction_add ChatWindow::signal_action_reaction_add() {
    return m_signal_action_reaction_add;
}

ChatWindow::type_signal_action_reaction_remove ChatWindow::signal_action_reaction_remove() {
    return m_signal_action_reaction_remove;
}
