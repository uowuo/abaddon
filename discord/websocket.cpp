#include "websocket.hpp"
#include <functional>

Websocket::Websocket() {}

void Websocket::StartConnection(std::string url) {
    m_websocket.disableAutomaticReconnection();
    m_websocket.setUrl(url);
    m_websocket.setOnMessageCallback(std::bind(&Websocket::OnMessage, this, std::placeholders::_1));
    m_websocket.setExtraHeaders(ix::WebSocketHttpHeaders { { "User-Agent", m_agent } }); // idk if this actually works
    m_websocket.start();
}

void Websocket::SetUserAgent(std::string agent) {
    m_agent = agent;
}

void Websocket::Stop() {
    m_websocket.stop();
}

void Websocket::Stop(uint16_t code) {
    m_websocket.stop(code);
}

bool Websocket::IsOpen() const {
    auto state = m_websocket.getReadyState();
    return state == ix::ReadyState::Open;
}

void Websocket::Send(const std::string &str) {
    printf("sending %s\n", str.c_str());
    m_websocket.sendText(str);
}

void Websocket::Send(const nlohmann::json &j) {
    Send(j.dump());
}

void Websocket::OnMessage(const ix::WebSocketMessagePtr &msg) {
    switch (msg->type) {
        case ix::WebSocketMessageType::Open: {
            m_signal_open.emit();
        } break;
        case ix::WebSocketMessageType::Close: {
            m_signal_close.emit(msg->closeInfo.code);
        } break;
        case ix::WebSocketMessageType::Message: {
            m_signal_message.emit(msg->str);
        } break;
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
