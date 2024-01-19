#include "chatwindow.hpp"
#include "abaddon.hpp"
#include "chatinputindicator.hpp"
#include "ratelimitindicator.hpp"
#include "chatinput.hpp"
#include "chatlist.hpp"
#include "constants.hpp"
#ifdef WITH_LIBHANDY
#include "channeltabswitcherhandy.hpp"
#endif

ChatWindow::ChatWindow() {
    auto &discord = Abaddon::Get().GetDiscordClient();
    discord.signal_message_send_fail().connect(sigc::mem_fun(*this, &ChatWindow::OnMessageSendFail));

    m_main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    m_chat = Gtk::manage(new ChatList);
    m_input = Gtk::manage(new ChatInput);
    m_input_indicator = Gtk::manage(new ChatInputIndicator);
    m_rate_limit_indicator = Gtk::manage(new RateLimitIndicator);
    m_meta = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));

#ifdef WITH_LIBHANDY
    m_tab_switcher = Gtk::make_managed<ChannelTabSwitcherHandy>();
    m_tab_switcher->signal_channel_switched_to().connect([this](Snowflake id) {
        m_signal_action_channel_click.emit(id, false);
    });
#endif

    m_rate_limit_indicator->set_margin_end(5);
    m_rate_limit_indicator->set_hexpand(true);
    m_rate_limit_indicator->set_halign(Gtk::ALIGN_END);
    m_rate_limit_indicator->set_valign(Gtk::ALIGN_END);
    m_rate_limit_indicator->show();

    m_input_indicator->set_halign(Gtk::ALIGN_START);
    m_input_indicator->set_valign(Gtk::ALIGN_END);
    m_input_indicator->show();

    m_main->get_style_context()->add_class("messages");

    m_main->set_hexpand(true);
    m_main->set_vexpand(true);

    m_topic.get_style_context()->add_class("channel-topic");
    m_topic.add(m_topic_text);
    m_topic_text.set_halign(Gtk::ALIGN_START);
    m_topic_text.show();

    m_input->set_valign(Gtk::ALIGN_END);

    m_input->signal_submit().connect(sigc::mem_fun(*this, &ChatWindow::OnInputSubmit));
    m_input->signal_escape().connect([this]() {
        if (m_is_replying) StopReplying();
        if (m_is_editing) StopEditing();
    });
    m_input->signal_key_press_event().connect(sigc::mem_fun(*this, &ChatWindow::OnKeyPressEvent), false);
    m_input->show();

    m_completer.SetBuffer(m_input->GetBuffer());
    m_completer.SetGetChannelID([this]() {
        return m_active_channel;
    });

    m_completer.SetGetRecentAuthors([this]() {
        return m_chat->GetRecentAuthors();
    });

    m_completer.show();

    m_chat->signal_action_channel_click().connect([this](Snowflake id) {
        m_signal_action_channel_click.emit(id, true);
    });
    m_chat->signal_action_chat_load_history().connect([this](Snowflake id) {
        m_signal_action_chat_load_history.emit(id);
    });
    m_chat->signal_action_insert_mention().connect([this](Snowflake id) {
        // lowkey gross
        m_signal_action_insert_mention.emit(id);
    });
    m_chat->signal_action_message_edit().connect([this](Snowflake channel_id, Snowflake message_id) {
        m_signal_action_message_edit.emit(channel_id, message_id);
    });
    m_chat->signal_action_reaction_add().connect([this](Snowflake id, const Glib::ustring &param) {
        m_signal_action_reaction_add.emit(id, param);
    });
    m_chat->signal_action_reaction_remove().connect([this](Snowflake id, const Glib::ustring &param) {
        m_signal_action_reaction_remove.emit(id, param);
    });
    m_chat->signal_action_reply_to().connect([this](Snowflake id) {
        StartReplying(id);
    });
    m_chat->show();

    m_meta->set_hexpand(true);
    m_meta->set_halign(Gtk::ALIGN_FILL);
    m_meta->show();

    m_meta->add(*m_input_indicator);
    m_meta->add(*m_rate_limit_indicator);
    // m_scroll->add(*m_list);
#ifdef WITH_LIBHANDY
    m_main->add(*m_tab_switcher);
    m_tab_switcher->show();
#endif
    m_main->add(m_topic);
    m_main->add(*m_chat);
    m_main->add(m_completer);
    m_main->add(*m_input);
    m_main->add(*m_meta);
    m_main->add(m_progress);

    m_progress.signal_start().connect([this]() {
        m_progress.show();
    });

    m_progress.signal_stop().connect([this]() {
        m_progress.hide();
    });

    m_main->show();
}

Gtk::Widget *ChatWindow::GetRoot() const {
    return m_main;
}

void ChatWindow::Clear() {
    m_chat->Clear();
}

void ChatWindow::SetMessages(const std::vector<Message> &msgs) {
    m_chat->SetMessages(msgs.begin(), msgs.end());
}

void ChatWindow::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
    m_chat->SetActiveChannel(id);
    m_input->SetActiveChannel(id);
    m_input_indicator->SetActiveChannel(id);
    m_rate_limit_indicator->SetActiveChannel(id);
    if (m_is_replying) StopReplying();
    if (m_is_editing) StopEditing();

#ifdef WITH_LIBHANDY
    m_tab_switcher->ReplaceActiveTab(id);
#endif
}

void ChatWindow::AddNewMessage(const Message &data) {
    m_chat->ProcessNewMessage(data, false);
}

void ChatWindow::DeleteMessage(Snowflake id) {
    m_chat->DeleteMessage(id);
}

void ChatWindow::UpdateMessage(Snowflake id) {
    m_chat->RefetchMessage(id);
}

void ChatWindow::AddNewHistory(const std::vector<Message> &msgs) {
    m_chat->PrependMessages(msgs.crbegin(), msgs.crend());
}

void ChatWindow::InsertChatInput(const std::string &text) {
    m_input->InsertText(text);
}

Snowflake ChatWindow::GetOldestListedMessage() {
    return m_chat->GetOldestListedMessage();
}

void ChatWindow::UpdateReactions(Snowflake id) {
    m_chat->UpdateMessageReactions(id);
}

void ChatWindow::SetTopic(const std::string &text) {
    m_topic_text.set_text(text);
    m_topic.set_visible(text.length() > 0);
}

void ChatWindow::AddAttachment(const Glib::RefPtr<Gio::File> &file) {
    m_input->AddAttachment(file);
}

#ifdef WITH_LIBHANDY
void ChatWindow::OpenNewTab(Snowflake id) {
    // open if its the first tab (in which case it really isnt a tab but whatever)
    if (m_tab_switcher->GetNumberOfTabs() == 0) {
        m_signal_action_channel_click.emit(id, false);
    }
    m_tab_switcher->AddChannelTab(id);
}

TabsState ChatWindow::GetTabsState() {
    return m_tab_switcher->GetTabsState();
}

void ChatWindow::UseTabsState(const TabsState &state) {
    m_tab_switcher->UseTabsState(state);
}

void ChatWindow::GoBack() {
    m_tab_switcher->GoBackOnCurrent();
}

void ChatWindow::GoForward() {
    m_tab_switcher->GoForwardOnCurrent();
}

void ChatWindow::GoToPreviousTab() {
    m_tab_switcher->GoToPreviousTab();
}

void ChatWindow::GoToNextTab() {
    m_tab_switcher->GoToNextTab();
}

void ChatWindow::GoToTab(int idx) {
    m_tab_switcher->GoToTab(idx);
}
#endif

Snowflake ChatWindow::GetActiveChannel() const {
    return m_active_channel;
}

bool ChatWindow::OnInputSubmit(ChatSubmitParams data) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    if (!discord.HasSelfChannelPermission(m_active_channel, Permission::SEND_MESSAGES)) return false;
    if (!data.Attachments.empty() && !discord.HasSelfChannelPermission(m_active_channel, Permission::ATTACH_FILES)) return false;

    int nitro_restriction = BaseAttachmentSizeLimit;
    const auto nitro = discord.GetUserData().PremiumType;
    if (!nitro.has_value() || nitro == EPremiumType::None) {
        nitro_restriction = BaseAttachmentSizeLimit;
    } else if (nitro == EPremiumType::NitroClassic) {
        nitro_restriction = NitroClassicAttachmentSizeLimit;
    } else if (nitro == EPremiumType::Nitro) {
        nitro_restriction = NitroAttachmentSizeLimit;
    }

    int guild_restriction = BaseAttachmentSizeLimit;
    if (const auto channel = discord.GetChannel(m_active_channel); channel.has_value() && channel->GuildID.has_value()) {
        if (const auto guild = discord.GetGuild(*channel->GuildID); guild.has_value()) {
            if (!guild->PremiumTier.has_value() || guild->PremiumTier == GuildPremiumTier::NONE || guild->PremiumTier == GuildPremiumTier::TIER_1) {
                guild_restriction = BaseAttachmentSizeLimit;
            } else if (guild->PremiumTier == GuildPremiumTier::TIER_2) {
                guild_restriction = BoostLevel2AttachmentSizeLimit;
            } else if (guild->PremiumTier == GuildPremiumTier::TIER_3) {
                guild_restriction = BoostLevel3AttachmentSizeLimit;
            }
        }
    }

    int restriction = std::max(nitro_restriction, guild_restriction);

    goffset total_size = 0;
    for (const auto &attachment : data.Attachments) {
        const auto info = attachment.File->query_info();
        if (info) {
            const auto size = info->get_size();
            if (size > restriction) {
                m_input->IndicateTooLarge();
                return false;
            }
            total_size += size;
            if (total_size > MaxMessagePayloadSize) {
                m_input->IndicateTooLarge();
                return false;
            }
        }
    }

    if (!m_rate_limit_indicator->CanSpeak())
        return false;

    if (data.Message.empty() && data.Attachments.empty())
        return false;

    data.ChannelID = m_active_channel;
    data.InReplyToID = m_replying_to;
    data.EditingID = m_editing_id;

    if (m_active_channel.IsValid())
        m_signal_action_chat_submit.emit(data); // m_replying_to is checked for invalid in the handler

    if (m_is_replying) StopReplying();
    if (m_is_editing) StopEditing();

    return true;
}

bool ChatWindow::ProcessKeyEvent(GdkEventKey *e) {
    if (e->type != GDK_KEY_PRESS) return false;
    if (e->keyval == GDK_KEY_Up && !(e->state & GDK_SHIFT_MASK) && m_input->IsEmpty()) {
        const auto edit_id = m_chat->GetLastSentEditableMessage();
        if (edit_id.has_value()) {
            StartEditing(*edit_id);
        }

        return true;
    }

    return false;
}

bool ChatWindow::OnKeyPressEvent(GdkEventKey *e) {
    if (m_completer.ProcessKeyPress(e))
        return true;

    if (m_input->ProcessKeyPress(e))
        return true;

    if (ProcessKeyEvent(e))
        return true;

    return false;
}

void ChatWindow::StartReplying(Snowflake message_id) {
    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto message = *discord.GetMessage(message_id);
    const auto author = discord.GetUser(message.Author.ID);
    m_replying_to = message_id;
    m_is_replying = true;
    m_input->StartReplying();
    if (author.has_value()) {
        m_input_indicator->SetCustomMarkup("Replying to " + author->GetUsernameEscapedBold());
    } else {
        m_input_indicator->SetCustomMarkup("Replying...");
    }
}

void ChatWindow::StopReplying() {
    m_is_replying = false;
    m_replying_to = Snowflake::Invalid;
    m_input->StopReplying();
    m_input_indicator->ClearCustom();
}

void ChatWindow::StartEditing(Snowflake message_id) {
    const auto message = Abaddon::Get().GetDiscordClient().GetMessage(message_id);
    if (!message.has_value()) {
        spdlog::get("ui")->warn("ChatWindow::StartEditing message is nullopt");
        return;
    }
    m_is_editing = true;
    m_editing_id = message_id;
    m_input->StartEditing(*message);
    m_input_indicator->SetCustomMarkup("Editing...");
}

void ChatWindow::StopEditing() {
    m_is_editing = false;
    m_editing_id = Snowflake::Invalid;
    m_input->StopEditing();
    m_input->Clear();
    m_input_indicator->ClearCustom();
}

void ChatWindow::OnScrollEdgeOvershot(Gtk::PositionType pos) {
    if (pos == Gtk::POS_TOP)
        m_signal_action_chat_load_history.emit(m_active_channel);
}

void ChatWindow::OnMessageSendFail(const std::string &nonce, float retry_after) {
    m_chat->SetFailedByNonce(nonce);
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

ChatWindow::type_signal_action_reaction_add ChatWindow::signal_action_reaction_add() {
    return m_signal_action_reaction_add;
}

ChatWindow::type_signal_action_reaction_remove ChatWindow::signal_action_reaction_remove() {
    return m_signal_action_reaction_remove;
}
