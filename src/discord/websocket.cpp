#include "websocket.hpp"

#include <gtkmm/main.h>
#include <spdlog/sinks/stdout_color_sinks.h>

Websocket::Websocket(const std::string &id) {
    if (m_log = spdlog::get(id); !m_log) {
        m_log = spdlog::stdout_color_mt(id);
    }

    m_websocket = curl_easy_init();
    curl_easy_setopt(m_websocket, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(m_websocket, CURLOPT_CONNECT_ONLY, 2L);

    m_open_dispatcher.connect([this]() {
        m_signal_open.emit();
    });

    m_close_dispatcher.connect([this]() {
        m_signal_close.emit(m_close_info);
    });
}

void Websocket::StartConnection(const std::string &url) {
    m_log->debug("Starting connection to {}", url);

    m_sent_close = false;
    m_state = State::Connecting;

    std::scoped_lock lock(m_mutex);
    curl_easy_setopt(m_websocket, CURLOPT_URL, url.c_str());

    m_thread = std::thread(&Websocket::Task, this);
}

void Websocket::SetUserAgent(std::string agent) {
    std::scoped_lock lock(m_mutex);
    curl_easy_setopt(m_websocket, CURLOPT_USERAGENT, agent.c_str());
}

bool Websocket::GetPrintMessages() const noexcept {
    return m_print_messages;
}

void Websocket::SetPrintMessages(bool show) noexcept {
    m_print_messages = show;
}

void Websocket::Stop() {
    m_log->debug("Stopping with default close code");
    Stop(1000);
}

void Websocket::Stop(uint16_t code) {
    if (m_state == State::Closing) {
        m_log->debug("Attempting to stop while in Closing state");
        return;
    }

    if (m_state == State::Closed) {
        m_log->debug("Attempting to stop while in Closed state");
        return;
    }

    m_log->debug("Stopping with close code {}", code);

    m_state = State::Closing;

    {
        std::vector<uint8_t> payload = { static_cast<uint8_t>(code >> 8), static_cast<uint8_t>(code & 0xFF) };
        m_sent_close = true;
        {
            std::scoped_lock lock(m_mutex);
            size_t sent;
            curl_ws_send(m_websocket, payload.data(), payload.size(), &sent, 0, CURLWS_CLOSE);
        }
        std::unique_lock lock(m_close_mutex);
        m_close_cv.wait(lock);
    }

    m_log->trace("Socket::stop complete");
    while (Gtk::Main::events_pending()) {
        Gtk::Main::iteration();
    }
    m_log->trace("No events pending");
}

void Websocket::Send(const std::string &str) {
    if (m_state != State::Open) {
        m_log->warn("Attempt to send on non-open state");
        return;
    }

    if (m_print_messages)
        m_log->trace("Send: {}", str);

    std::scoped_lock lock(m_mutex);

    size_t offset = 0;
    while (true) {
        size_t sent;
        auto code = curl_ws_send(m_websocket, str.data() + offset, str.size() - offset, &sent, 0, CURLWS_TEXT);
        offset += sent;
        if (code == CURLE_OK) {
            if (offset >= str.size()) {
                break;
            }
        } else if (code == CURLE_AGAIN) {
            continue;
        } else {
            m_log->error("Websocket::Send: {}", curl_easy_strerror(code));
            break;
        }
    }
}

void Websocket::Send(const nlohmann::json &j) {
    Send(j.dump());
}

void Websocket::OnMessage(const WebSocketMessage &message) {
    switch (message.Type) {
        case WebSocketMessage::MessageType::binary:
        case WebSocketMessage::MessageType::text: {
            m_log->trace("Received {} bytes", message.Data.size());
            std::string payload = std::string(message.Data.begin(), message.Data.end());
            m_signal_message.emit(payload);
            break;
        }
        case WebSocketMessage::MessageType::close: {
            m_close_info = message.CloseInfo;
            m_close_info.Remote = !m_sent_close;

            m_log->debug(
                "Received close frame{}, dispatching. {} ({})",
                m_close_info.Remote ? " (Remote)" : "",
                message.CloseInfo.Code,
                message.CloseInfo.Reason);
            m_close_cv.notify_all();
            m_close_dispatcher.emit();
            break;
        }
        default:
            break;
    }
}

std::expected<Websocket::WebSocketMessage, Websocket::Error> Websocket::ReceiveMessage() {
    WebSocketMessage message;

    curl_socket_t socket;
    {
        std::scoped_lock lock(m_mutex);
        curl_easy_getinfo(m_websocket, CURLINFO_ACTIVESOCKET, &socket);
    }

    while (m_state == State::Open || m_state == State::Closing) {
        if (m_state == State::Closing && !m_sent_close) {
            // if were closing and we didnt send the close then server already did
            return std::unexpected(Error::closing);
        }

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(socket, &rfds);

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        if (select(socket + 1, &rfds, nullptr, nullptr, &tv) == -1) {
            return std::unexpected(Websocket::Error::select_error);
        }

        size_t rlen;
        const curl_ws_frame *meta;
        char buffer[256];

        std::scoped_lock lock(m_mutex);

        auto code = curl_ws_recv(m_websocket, buffer, sizeof(buffer), &rlen, &meta);
        if (code != CURLE_OK) {
            if (code == CURLE_AGAIN) {
                continue;
            } else {
                return std::unexpected(Websocket::Error::recv_error);
            }
        }

        if (meta->flags & CURLWS_PING) {
            message.Type = WebSocketMessage::MessageType::ping;
        } else if (meta->flags & CURLWS_TEXT) {
            message.Type = WebSocketMessage::MessageType::text;
        } else if (meta->flags & CURLWS_BINARY) {
            message.Type = WebSocketMessage::MessageType::binary;
        } else if (meta->flags & CURLWS_CLOSE) {
            message.Type = WebSocketMessage::MessageType::close;
            message.CloseInfo.Code = (static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint8_t>(buffer[1]);
            message.CloseInfo.Reason = std::string(buffer + 2, rlen - 2);
            if (m_sent_close) {
                m_state = State::Closed;
            } else {
                m_state = State::Closing;
            }
        }

        message.Data.insert(message.Data.end(), buffer, buffer + rlen);

        if (meta->bytesleft == 0 && (meta->flags & CURLWS_CONT) == 0) {
            break;
        }
    }

    return message;
}

void Websocket::Task() {
    CURLcode res;

    {
        std::scoped_lock lock(m_mutex);
        res = curl_easy_perform(m_websocket);
    }
    if (res != CURLE_OK) {
        m_state = State::Closed;
        m_log->error("Error: {}", curl_easy_strerror(res));
    } else {
        m_state = State::Open;
        m_open_dispatcher.emit();

        while (true) {
            auto msg = ReceiveMessage();
            if (msg.has_value()) {
                OnMessage(msg.value());
            }
        }
    }

    std::scoped_lock lock(m_mutex);
    curl_easy_cleanup(m_websocket);
}

Websocket::type_signal_open Websocket::signal_open() {
    return m_signal_open;
}

Websocket::type_signal_close Websocket::signal_close() {
    return m_signal_close;
}

Websocket::type_signal_message Websocket::signal_message() {
    return m_signal_message;
}
