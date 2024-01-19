#include "progressbar.hpp"

#include "abaddon.hpp"

MessageUploadProgressBar::MessageUploadProgressBar() {
    get_style_context()->add_class("message-progress");
    auto &discord = Abaddon::Get().GetDiscordClient();
    discord.signal_message_progress().connect([this](const std::string &nonce, float percent) {
        if (nonce == m_last_nonce) {
            if (!m_active) {
                m_active = true;
                m_signal_start.emit();
            }
            set_fraction(percent);
        }
    });
    discord.signal_message_send_fail().connect([this](const std::string &nonce, float) {
        if (nonce == m_last_nonce) {
            set_fraction(0.0);
            m_active = false;
            m_signal_stop.emit();
        }
    });
    discord.signal_message_create().connect([this](const Message &msg) {
        if (msg.IsPending) {
            m_last_nonce = *msg.Nonce;
        } else if (msg.Nonce.has_value() && (*msg.Nonce == m_last_nonce)) {
            m_last_nonce = "";
            set_fraction(0.0);
            m_active = false;
            m_signal_stop.emit();
        }
    });
}

MessageUploadProgressBar::type_signal_start MessageUploadProgressBar::signal_start() {
    return m_signal_start;
}

MessageUploadProgressBar::type_signal_stop MessageUploadProgressBar::signal_stop() {
    return m_signal_stop;
}
