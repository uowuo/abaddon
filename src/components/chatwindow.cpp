#include "chatwindow.hpp"
#include "chatmessage.hpp"
#include "abaddon.hpp"
#include "chatinputindicator.hpp"
#include "ratelimitindicator.hpp"
#include "chatinput.hpp"
#include "chatlist.hpp"

ChatWindow::ChatWindow() {
    Abaddon::Get().GetDiscordClient().signal_message_send_fail().connect(sigc::mem_fun(*this, &ChatWindow::OnMessageSendFail));

    m_main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    m_chat = Gtk::manage(new ChatList);
    m_input = Gtk::manage(new ChatInput);
    m_input_indicator = Gtk::manage(new ChatInputIndicator);
    m_rate_limit_indicator = Gtk::manage(new RateLimitIndicator);
    m_meta = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));

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

    m_input->signal_submit().connect(sigc::mem_fun(*this, &ChatWindow::OnInputSubmit));
    m_input->signal_escape().connect([this]() {
        if (m_is_replying)
            StopReplying();
    });
    m_input->signal_key_press_event().connect(sigc::mem_fun(*this, &ChatWindow::OnKeyPressEvent), false);
    m_input->show();

    m_completer.SetBuffer(m_input->GetBuffer());
    m_completer.SetGetChannelID([this]() -> auto {
        return m_active_channel;
    });

    m_completer.SetGetRecentAuthors([this]() -> auto {
        return m_chat->GetRecentAuthors();
    });

    m_completer.show();

    m_chat->signal_action_channel_click().connect([this](Snowflake id) {
        m_signal_action_channel_click.emit(id);
    });
    m_chat->signal_action_chat_load_history().connect([this](Snowflake id) {
        m_signal_action_chat_load_history.emit(id);
    });
    m_chat->signal_action_chat_submit().connect([this](const std::string &str, Snowflake channel_id, Snowflake referenced_id) {
        m_signal_action_chat_submit.emit(str, channel_id, referenced_id);
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
    m_main->add(m_topic);
    m_main->add(*m_chat);
    m_main->add(m_completer);
    m_main->add(*m_input);
    m_main->add(*m_meta);
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
    m_input_indicator->SetActiveChannel(id);
    m_rate_limit_indicator->SetActiveChannel(id);
    if (m_is_replying)
        StopReplying();
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

void ChatWindow::InsertChatInput(std::string text) {
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

Snowflake ChatWindow::GetActiveChannel() const {
    return m_active_channel;
}

bool ChatWindow::OnInputSubmit(const Glib::ustring &text) {
    if (!m_rate_limit_indicator->CanSpeak())
        return false;

    if (text.size() == 0)
        return false;

    if (m_active_channel.IsValid())
        m_signal_action_chat_submit.emit(text, m_active_channel, m_replying_to); // m_replying_to is checked for invalid in the handler
    if (m_is_replying)
        StopReplying();

    return true;
}

bool ChatWindow::OnKeyPressEvent(GdkEventKey *e) {
    if (m_completer.ProcessKeyPress(e))
        return true;

    if (m_input->ProcessKeyPress(e))
        return true;

    return false;
}

void ChatWindow::StartReplying(Snowflake message_id) {
    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto message = *discord.GetMessage(message_id);
    const auto author = discord.GetUser(message.Author.ID);
    m_replying_to = message_id;
    m_is_replying = true;
    m_input->grab_focus();
    m_input->get_style_context()->add_class("replying");
    if (author.has_value())
        m_input_indicator->SetCustomMarkup("Replying to " + author->GetEscapedBoldString<false>());
    else
        m_input_indicator->SetCustomMarkup("Replying...");
}

void ChatWindow::StopReplying() {
    m_is_replying = false;
    m_replying_to = Snowflake::Invalid;
    m_input->get_style_context()->remove_class("replying");
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
