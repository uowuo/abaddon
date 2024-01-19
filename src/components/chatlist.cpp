#include "chatlist.hpp"
#include "abaddon.hpp"
#include "chatmessage.hpp"
#include "constants.hpp"

ChatList::ChatList() {
    m_list.get_style_context()->add_class("messages");

    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);

    get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &ChatList::OnVAdjustmentValueChanged));
    get_vadjustment()->property_upper().signal_changed().connect(sigc::mem_fun(*this, &ChatList::OnVAdjustmentUpperChanged));

    m_list.signal_size_allocate().connect(sigc::mem_fun(*this, &ChatList::OnListSizeAllocate));

    m_list.set_focus_hadjustment(get_hadjustment());
    m_list.set_focus_vadjustment(get_vadjustment());
    m_list.set_selection_mode(Gtk::SELECTION_NONE);
    m_list.set_hexpand(true);
    m_list.set_vexpand(true);

    add(m_list);

    m_list.show();

    SetupMenu();
}

void ChatList::Clear() {
    auto children = m_list.get_children();
    auto it = children.begin();
    while (it != children.end()) {
        delete *it;
        it++;
    }
    m_id_to_widget.clear();
    m_num_messages = 0;
    m_num_rows = 0;
}

void ChatList::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
}

void ChatList::ProcessNewMessage(const Message &data, bool prepend) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (!discord.IsStarted()) return;
    if (!prepend) m_ignore_next_upper = true;

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
    if (!m_separate_all && m_num_rows > 0) {
        if (prepend)
            last_row = dynamic_cast<ChatMessageHeader *>(m_list.get_row_at_index(0));
        else
            last_row = dynamic_cast<ChatMessageHeader *>(m_list.get_row_at_index(m_num_rows - 1));

        if (last_row != nullptr) {
            const uint64_t diff = std::max(data.ID, last_row->NewestID) - std::min(data.ID, last_row->NewestID);
            if (last_row->UserID == data.Author.ID && (prepend || (diff < SnowflakeSplitDifference * Snowflake::SecondsInterval))) {
                should_attach = true;
            }
            // Separate webhooks if the usernames or avatar URLs are different
            if (data.IsWebhook() && last_row->UserID == data.Author.ID) {
                const auto last_message = discord.GetMessage(last_row->NewestID);
                if (last_message.has_value() && last_message->IsWebhook()) {
                    const auto last_webhook_data = last_message->GetWebhookData();
                    const auto next_webhook_data = data.GetWebhookData();
                    if (last_webhook_data.has_value() && next_webhook_data.has_value()) {
                        if (last_webhook_data->Username != next_webhook_data->Username || last_webhook_data->Avatar != next_webhook_data->Avatar) {
                            should_attach = false;
                        }
                    }
                }
            }
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
        const auto chan = discord.GetChannel(m_active_channel);
        Snowflake guild_id;
        if (chan.has_value() && chan->GuildID.has_value())
            guild_id = *chan->GuildID;
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

        const auto cb = [this, id = data.ID](GdkEventButton *ev) -> bool {
            if (ev->type == GDK_BUTTON_PRESS && ev->button == GDK_BUTTON_SECONDARY) {
                m_menu_selected_message = id;

                const auto &client = Abaddon::Get().GetDiscordClient();
                const auto data = client.GetMessage(id);
                if (!data.has_value()) return false;
                const auto channel = client.GetChannel(m_active_channel);

                bool has_manage = channel.has_value() && (channel->Type == ChannelType::DM || channel->Type == ChannelType::GROUP_DM);
                if (!has_manage)
                    has_manage = client.HasChannelPermission(client.GetUserData().ID, m_active_channel, Permission::MANAGE_MESSAGES);

                m_menu_edit_message->set_visible(!m_use_pinned_menu);
                m_menu_reply_to->set_visible(!m_use_pinned_menu);
                m_menu_unpin->set_visible(has_manage && data->IsPinned);
                m_menu_pin->set_visible(has_manage && !data->IsPinned);

                if (data->IsDeleted()) {
                    m_menu_delete_message->set_sensitive(false);
                    m_menu_edit_message->set_sensitive(false);
                } else {
                    const bool can_delete = (client.GetUserData().ID == data->Author.ID) || has_manage;
                    m_menu_delete_message->set_sensitive(can_delete);
                    m_menu_edit_message->set_sensitive(data->IsEditable());
                }

                m_menu.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
            }
            return false;
        };
        content->signal_button_press_event().connect(cb);

        if (!data.IsPending) {
            content->signal_action_reaction_add().connect([this, id = data.ID](const Glib::ustring &param) {
                m_signal_action_reaction_add.emit(id, param);
            });
            content->signal_action_reaction_remove().connect([this, id = data.ID](const Glib::ustring &param) {
                m_signal_action_reaction_remove.emit(id, param);
            });
            content->signal_action_channel_click().connect([this](const Snowflake &id) {
                m_signal_action_channel_click.emit(id);
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

    if (x != nullptr) {
        if (Abaddon::Get().GetSettings().ShowDeletedIndicator) {
            x->UpdateAttributes();
        } else {
            RemoveMessageAndHeader(x);
            m_id_to_widget.erase(id);
        }
    }
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
    if (!m_id_to_widget.empty())
        return m_id_to_widget.begin()->first;
    else
        return Snowflake::Invalid;
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

void ChatList::SetSeparateAll(bool separate) {
    m_separate_all = true;
}

void ChatList::SetUsePinnedMenu() {
    m_use_pinned_menu = true;
}

void ChatList::ActuallyRemoveMessage(Snowflake id) {
    auto it = m_id_to_widget.find(id);
    if (it != m_id_to_widget.end())
        RemoveMessageAndHeader(it->second);
}

std::optional<Snowflake> ChatList::GetLastSentEditableMessage() {
    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto self_id = discord.GetUserData().ID;

    std::map<Snowflake, Gtk::Widget *> ordered(m_id_to_widget.begin(), m_id_to_widget.end());

    for (auto it = ordered.crbegin(); it != ordered.crend(); it++) {
        const auto *widget = dynamic_cast<ChatMessageItemContainer *>(it->second);
        if (widget == nullptr) continue;
        const auto msg = discord.GetMessage(widget->ID);
        if (!msg.has_value()) continue;
        if (msg->Author.ID != self_id) continue;
        if (!msg->IsEditable()) continue;
        return msg->ID;
    }

    return std::nullopt;
}

void ChatList::SetupMenu() {
    m_menu_copy_id = Gtk::manage(new Gtk::MenuItem("Copy ID"));
    m_menu_copy_id->signal_activate().connect([this] {
        Gtk::Clipboard::get()->set_text(std::to_string(m_menu_selected_message));
    });
    m_menu_copy_id->show();
    m_menu.append(*m_menu_copy_id);

    m_menu_delete_message = Gtk::manage(new Gtk::MenuItem("Delete Message"));
    m_menu_delete_message->signal_activate().connect([this] {
        Abaddon::Get().GetDiscordClient().DeleteMessage(m_active_channel, m_menu_selected_message);
    });
    m_menu_delete_message->show();
    m_menu.append(*m_menu_delete_message);

    m_menu_edit_message = Gtk::manage(new Gtk::MenuItem("Edit Message"));
    m_menu_edit_message->signal_activate().connect([this] {
        m_signal_action_message_edit.emit(m_active_channel, m_menu_selected_message);
    });
    m_menu_edit_message->show();
    m_menu.append(*m_menu_edit_message);

    m_menu_copy_content = Gtk::manage(new Gtk::MenuItem("Copy Content"));
    m_menu_copy_content->signal_activate().connect([this] {
        const auto msg = Abaddon::Get().GetDiscordClient().GetMessage(m_menu_selected_message);
        if (msg.has_value())
            Gtk::Clipboard::get()->set_text(msg->Content);
    });
    m_menu_copy_content->show();
    m_menu.append(*m_menu_copy_content);

    m_menu_reply_to = Gtk::manage(new Gtk::MenuItem("Reply To"));
    m_menu_reply_to->signal_activate().connect([this] {
        m_signal_action_reply_to.emit(m_menu_selected_message);
    });
    m_menu_reply_to->show();
    m_menu.append(*m_menu_reply_to);

    m_menu_unpin = Gtk::manage(new Gtk::MenuItem("Unpin"));
    m_menu_unpin->signal_activate().connect([this] {
        Abaddon::Get().GetDiscordClient().Unpin(m_active_channel, m_menu_selected_message, [](...) {});
    });
    m_menu.append(*m_menu_unpin);

    m_menu_pin = Gtk::manage(new Gtk::MenuItem("Pin"));
    m_menu_pin->signal_activate().connect([this] {
        Abaddon::Get().GetDiscordClient().Pin(m_active_channel, m_menu_selected_message, [](...) {});
    });
    m_menu.append(*m_menu_pin);

    m_menu.show();
}

void ChatList::ScrollToBottom() {
    auto v = get_vadjustment();
    v->set_value(v->get_upper());
}

void ChatList::OnVAdjustmentValueChanged() {
    auto v = get_vadjustment();
    if (m_history_timer.elapsed() > 1 && v->get_value() < 500) {
        m_history_timer.start();
        m_signal_action_chat_load_history.emit(m_active_channel);
    }
    m_should_scroll_to_bottom = v->get_upper() - v->get_page_size() <= v->get_value();
}

void ChatList::OnVAdjustmentUpperChanged() {
    auto v = get_vadjustment();
    if (!m_ignore_next_upper && !m_should_scroll_to_bottom && m_old_upper > -1.0) {
        const auto inc = v->get_upper() - m_old_upper;
        v->set_value(v->get_value() + inc);
    }
    m_ignore_next_upper = false;
    m_old_upper = v->get_upper();
}

void ChatList::OnListSizeAllocate(Gtk::Allocation &allocation) {
    if (m_should_scroll_to_bottom) ScrollToBottom();
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

ChatList::type_signal_action_message_edit ChatList::signal_action_message_edit() {
    return m_signal_action_message_edit;
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
