#include "websocket.hpp"
#include <functional>
#include <nlohmann/json.hpp>

Websocket::Websocket() {}

void Websocket::StartConnection(std::string url) {
    m_websocket.setUrl(url);
    m_websocket.setOnMessageCallback(std::bind(&Websocket::OnMessage, this, std::placeholders::_1));
    m_websocket.start();
}

void Websocket::Stop() {
    m_websocket.stop();
}

bool Websocket::IsOpen() const {
    auto state = m_websocket.getReadyState();
    return state == ix::ReadyState::Open;
}

void Websocket::SetJSONCallback(JSONCallback_t func) {
    m_json_callback = func;
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
            //if (msg->str.size() > 1000)
            //    printf("%s\n", msg->str.substr(0, 1000).c_str());
            //else
            //    printf("%s\n", msg->str.c_str());
            auto obj = nlohmann::json::parse(msg->str);
            if (m_json_callback)
                m_json_callback(obj);
        } break;
    }
}
