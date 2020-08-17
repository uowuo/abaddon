#include "websocket.hpp"
#include <functional>
#include <nlohmann/json.hpp>

Websocket::Websocket() {}

void Websocket::StartConnection(std::string url) {
    m_websocket.setUrl(url);
    m_websocket.setOnMessageCallback(std::bind(&Websocket::OnMessage, this, std::placeholders::_1));
    m_websocket.start();
}

void Websocket::SetJSONCallback(JSONCallback_t func) {
    m_json_callback = func;
}

void Websocket::Send(const std::string &str) {
    m_websocket.sendText(str);
}

void Websocket::OnMessage(const ix::WebSocketMessagePtr &msg) {
    switch (msg->type) {
        case ix::WebSocketMessageType::Message:
            printf("%s\n", msg->str.c_str());
            auto obj = nlohmann::json::parse(msg->str);
            if (m_json_callback)
                m_json_callback(obj);
            break;
    }
}
