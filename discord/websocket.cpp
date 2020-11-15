#include "websocket.hpp"
#include <functional>

Websocket::Websocket() {}

void Websocket::StartConnection(std::string url) {
    m_websocket.disableAutomaticReconnection();
    m_websocket.setUrl(url);
    m_websocket.setOnMessageCallback(std::bind(&Websocket::OnMessage, this, std::placeholders::_1));
    m_websocket.start();
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

void Websocket::SetMessageCallback(MessageCallback_t func) {
    m_callback = func;
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
        case ix::WebSocketMessageType::Message: {
            if (m_callback)
                m_callback(msg->str);
        } break;
    }
}
