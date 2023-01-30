#include "progressbar.hpp"

MessageUploadProgressBar::MessageUploadProgressBar() {
    get_style_context()->add_class("message-progress");
    auto &discord = Abaddon::Get().GetDiscordClient();
    discord.signal_message_progress().connect([this](const std::string &nonce, float percent) {
        if (nonce == m_last_nonce) {
            set_fraction(percent);
        }
    });
    discord.signal_message_send_fail().connect([this](const std::string &nonce, float) {
        if (nonce == m_last_nonce)
            set_fraction(0.0);
    });
    discord.signal_message_create().connect([this](const Message &msg) {
        if (msg.IsPending) {
            m_last_nonce = *msg.Nonce;
        } else if (msg.Nonce.has_value() && (*msg.Nonce == m_last_nonce)) {
            m_last_nonce = "";
            set_fraction(0.0);
        }
    });
}
