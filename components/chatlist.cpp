#include "chatmessage.hpp"
#include "chatlist.hpp"
#include "../abaddon.hpp"
#include "../constants.hpp"

ChatList::ChatList() {
    m_list.get_style_context()->add_class("messages");

    set_can_focus(false);
    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
    signal_edge_reached().connect(sigc::mem_fun(*this, &ChatList::OnScrollEdgeOvershot));

    auto v = get_vadjustment();
    v->signal_value_changed().connect([this, v] {
        m_should_scroll_to_bottom = v->get_upper() - v->get_page_size() <= v->get_value();
    });

    m_list.signal_size_allocate().connect([this](Gtk::Allocation &) {
        if (m_should_scroll_to_bottom)
            ScrollToBottom();
    });

    m_list.set_focus_hadjustment(get_hadjustment());
    m_list.set_focus_vadjustment(get_vadjustment());
    m_list.set_selection_mode(Gtk::SELECTION_NONE);
    m_list.set_hexpand(true);
    m_list.set_vexpand(true);

    add(m_list);

    m_list.show();
}

void ChatList::Clear() {
    auto children = m_list.get_children();
    auto it = children.begin();
    while (it != children.end()) {
        delete *it;
        it++;
    }
}

void ChatList::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
}

void ChatList::ProcessNewMessage(const Message &data, bool prepend) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (!discord.IsStarted()) return;

    // delete preview message when gateway sends it back
    if (!data.IsPending && data.Nonce.has_value() && data.Author.ID == discord.GetUserData().ID) {
        for (auto [id, widget] : m_id_to_widget) {
            if (dynamic_cast<ChatMessageItemContainer *>(widget)->Nonce == *data.Nonce) {
                RemoveMessageAndHeader(widget);
                m_id_to_widget.erase(id);
                break;
            }
        }
    }

    ChatMessageHeader *last_row = nullptr;
    bool should_attach = false;
    if (m_num_rows > 0) {
        if (prepend)
            last_row = dynamic_cast<ChatMessageHeader *>(m_list.get_row_at_index(0));
        else
            last_row = dynamic_cast<ChatMessageHeader *>(m_list.get_row_at_index(m_num_rows - 1));

        if (last_row != nullptr) {
            const uint64_t diff = std::max(data.ID, last_row->NewestID) - std::min(data.ID, last_row->NewestID);
            if (last_row->UserID == data.Author.ID && (prepend || (diff < SnowflakeSplitDifference * Snowflake::SecondsInterval)))
                should_attach = true;
        }
    }

    m_num_messages++;

    if (m_should_scroll_to_bottom && !prepend) {
        while (m_num_messages > MaxMessagesForChatCull) {
            auto first_it = m_id_to_widget.begin();
            RemoveMessageAndHeader(first_it->second);
            m_id_to_widget.erase(first_it);
        }
    }

    ChatMessageHeader *header;
    if (should_attach) {
        header = last_row;
    } else {
        const auto guild_id = *discord.GetChannel(m_active_channel)->GuildID;
        const auto user_id = data.Author.ID;
        const auto user = discord.GetUser(user_id);
        if (!user.has_value()) return;

        header = Gtk::manage(new ChatMessageHeader(data));
        header->signal_action_insert_mention().connect([this, user_id]() {
            m_signal_action_insert_mention.emit(user_id);
        });

        header->signal_action_open_user_menu().connect([this, user_id, guild_id](const GdkEvent *event) {
            m_signal_action_open_user_menu.emit(event, user_id, guild_id);
        });

        m_num_rows++;
    }

    auto *content = ChatMessageItemContainer::FromMessage(data);
    if (content != nullptr) {
        header->AddContent(content, prepend);
        m_id_to_widget[data.ID] = content;

        if (!data.IsPending) {
            content->signal_action_delete().connect([this, id = data.ID] {
                m_signal_action_message_delete.emit(m_active_channel, id);
            });
            content->signal_action_edit().connect([this, id = data.ID] {
                m_signal_action_message_edit.emit(m_active_channel, id);
            });
            content->signal_action_reaction_add().connect([this, id = data.ID](const Glib::ustring &param) {
                m_signal_action_reaction_add.emit(id, param);
            });
            content->signal_action_reaction_remove().connect([this, id = data.ID](const Glib::ustring &param) {
                m_signal_action_reaction_remove.emit(id, param);
            });
            content->signal_action_channel_click().connect([this](const Snowflake &id) {
                m_signal_action_channel_click.emit(id);
            });
            content->signal_action_reply_to().connect([this](const Snowflake &id) {
                m_signal_action_reply_to.emit(id);
            });
        }
    }

    header->set_margin_left(5);
    header->show_all();

    if (!should_attach) {
        if (prepend)
            m_list.prepend(*header);
        else
            m_list.add(*header);
    }
}

void ChatList::DeleteMessage(Snowflake id) {
    auto widget = m_id_to_widget.find(id);
    if (widget == m_id_to_widget.end()) return;

    auto *x = dynamic_cast<ChatMessageItemContainer *>(widget->second);
    if (x != nullptr)
        x->UpdateAttributes();
}

void ChatList::RefetchMessage(Snowflake id) {
    auto widget = m_id_to_widget.find(id);
    if (widget == m_id_to_widget.end()) return;

    auto *x = dynamic_cast<ChatMessageItemContainer *>(widget->second);
    if (x != nullptr) {
        x->UpdateContent();
        x->UpdateAttributes();
    }
}

Snowflake ChatList::GetOldestListedMessage() {
    return m_id_to_widget.begin()->first;
}

void ChatList::UpdateMessageReactions(Snowflake id) {
    auto it = m_id_to_widget.find(id);
    if (it == m_id_to_widget.end()) return;
    auto *widget = dynamic_cast<ChatMessageItemContainer *>(it->second);
    if (widget == nullptr) return;
    widget->UpdateReactions();
}

void ChatList::SetFailedByNonce(const std::string &nonce) {
    for (auto [id, widget] : m_id_to_widget) {
        if (auto *container = dynamic_cast<ChatMessageItemContainer *>(widget); container->Nonce == nonce) {
            container->SetFailed();
            break;
        }
    }
}

std::vector<Snowflake> ChatList::GetRecentAuthors() {
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
}

void ChatList::OnScrollEdgeOvershot(Gtk::PositionType pos) {
    if (pos == Gtk::POS_TOP)
        m_signal_action_chat_load_history.emit(m_active_channel);
}

void ChatList::ScrollToBottom() {
    auto x = get_vadjustment();
    x->set_value(x->get_upper());
}

void ChatList::RemoveMessageAndHeader(Gtk::Widget *widget) {
    auto *header = dynamic_cast<ChatMessageHeader *>(widget->get_ancestor(Gtk::ListBoxRow::get_type()));
    if (header != nullptr) {
        if (header->GetChildContent().size() == 1) {
            m_num_rows--;
            delete header;
        } else {
            delete widget;
        }
    } else {
        delete widget;
    }
    m_num_messages--;
}

ChatList::type_signal_action_message_delete ChatList::signal_action_message_delete() {
    return m_signal_action_message_delete;
}

ChatList::type_signal_action_message_edit ChatList::signal_action_message_edit() {
    return m_signal_action_message_edit;
}

ChatList::type_signal_action_chat_submit ChatList::signal_action_chat_submit() {
    return m_signal_action_chat_submit;
}

ChatList::type_signal_action_chat_load_history ChatList::signal_action_chat_load_history() {
    return m_signal_action_chat_load_history;
}

ChatList::type_signal_action_channel_click ChatList::signal_action_channel_click() {
    return m_signal_action_channel_click;
}

ChatList::type_signal_action_insert_mention ChatList::signal_action_insert_mention() {
    return m_signal_action_insert_mention;
}

ChatList::type_signal_action_open_user_menu ChatList::signal_action_open_user_menu() {
    return m_signal_action_open_user_menu;
}

ChatList::type_signal_action_reaction_add ChatList::signal_action_reaction_add() {
    return m_signal_action_reaction_add;
}

ChatList::type_signal_action_reaction_remove ChatList::signal_action_reaction_remove() {
    return m_signal_action_reaction_remove;
}

ChatList::type_signal_action_reply_to ChatList::signal_action_reply_to() {
    return m_signal_action_reply_to;
}
