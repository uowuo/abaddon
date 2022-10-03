#include "websocket.hpp"
#include <utility>

Websocket::Websocket()
    : m_close_code(ix::WebSocketCloseConstants::kNormalClosureCode) {
    m_open_dispatcher.connect([this]() {
        m_signal_open.emit();
    });

    m_close_dispatcher.connect([this]() {
        Stop();
        m_signal_close.emit(m_close_code);
    });
}

void Websocket::StartConnection(const std::string &url) {
    m_websocket.disableAutomaticReconnection();
    m_websocket.setUrl(url);
    m_websocket.setOnMessageCallback([this](auto &&msg) { OnMessage(std::forward<decltype(msg)>(msg)); });
    m_websocket.setExtraHeaders(ix::WebSocketHttpHeaders { { "User-Agent", m_agent } }); // idk if this actually works
    m_websocket.start();
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
    Stop(ix::WebSocketCloseConstants::kNormalClosureCode);
}

void Websocket::Stop(uint16_t code) {
    m_websocket.stop(code);
}

void Websocket::Send(const std::string &str) {
    if (m_print_messages)
        printf("sending %s\n", str.c_str());
    m_websocket.sendText(str);
}

void Websocket::Send(const nlohmann::json &j) {
    Send(j.dump());
}

void Websocket::OnMessage(const ix::WebSocketMessagePtr &msg) {
    switch (msg->type) {
        case ix::WebSocketMessageType::Open: {
            m_open_dispatcher.emit();
        } break;
        case ix::WebSocketMessageType::Close: {
            m_close_code = msg->closeInfo.code;
            m_close_dispatcher.emit();
        } break;
        case ix::WebSocketMessageType::Message: {
            m_signal_message.emit(msg->str);
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
