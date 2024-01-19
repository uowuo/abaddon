#include "websocket.hpp"

#include <utility>

#include <gtkmm/main.h>
#include <spdlog/sinks/stdout_color_sinks.h>


Websocket::Websocket(const std::string &id)
    : m_close_info { 1000, "Normal", false } {
    if (m_log = spdlog::get(id); !m_log) {
        m_log = spdlog::stdout_color_mt(id);
    }

    m_open_dispatcher.connect([this]() {
        m_signal_open.emit();
    });

    m_close_dispatcher.connect([this]() {
        Stop();
        m_signal_close.emit(m_close_info);
    });
}

void Websocket::StartConnection(const std::string &url) {
    m_log->debug("Starting connection to {}", url);

    m_websocket = std::make_unique<ix::WebSocket>();

    m_websocket->disableAutomaticReconnection();
    m_websocket->setUrl(url);
    m_websocket->setOnMessageCallback([this](auto &&msg) { OnMessage(std::forward<decltype(msg)>(msg)); });
    m_websocket->setExtraHeaders(ix::WebSocketHttpHeaders { { "User-Agent", m_agent }, { "Origin", "https://discord.com" } }); // idk if this actually works
    m_websocket->start();
}

void Websocket::SetUserAgent(std::string agent) {
    m_agent = std::move(agent);
}

bool Websocket::GetPrintMessages() const noexcept {
    return m_print_messages;
}

void Websocket::SetPrintMessages(bool show) noexcept {
    m_print_messages = show;
}

void Websocket::Stop() {
    m_log->debug("Stopping with default close code");
    Stop(ix::WebSocketCloseConstants::kNormalClosureCode);
}

void Websocket::Stop(uint16_t code) {
    m_log->debug("Stopping with close code {}", code);
    m_websocket->stop(code);
    m_log->trace("Socket::stop complete");
    while (Gtk::Main::events_pending()) {
        Gtk::Main::iteration();
    }
    m_log->trace("No events pending");
}

void Websocket::Send(const std::string &str) {
    if (m_print_messages)
        m_log->trace("Send: {}", str);
    m_websocket->sendText(str);
}

void Websocket::Send(const nlohmann::json &j) {
    Send(j.dump());
}

void Websocket::OnMessage(const ix::WebSocketMessagePtr &msg) {
    switch (msg->type) {
        case ix::WebSocketMessageType::Open: {
            m_log->debug("Received open frame, dispatching");
            m_open_dispatcher.emit();
        } break;
        case ix::WebSocketMessageType::Close: {
            const auto remote = msg->closeInfo.remote ? " Remote" : "";
            m_log->debug("Received close frame, dispatching. {} ({}){}", msg->closeInfo.code, msg->closeInfo.reason, remote);
            m_close_info = msg->closeInfo;
            m_close_dispatcher.emit();
        } break;
        case ix::WebSocketMessageType::Message: {
            m_signal_message.emit(msg->str);
        } break;
        case ix::WebSocketMessageType::Error: {
            m_log->error("Websocket error: Status: {} Reason: {}", msg->errorInfo.http_status, msg->errorInfo.reason);
        } break;
        default:
            break;
    }
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
