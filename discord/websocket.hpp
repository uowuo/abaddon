#pragma once
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

class Websocket {
public:
    Websocket();
    void StartConnection(std::string url);

    using JSONCallback_t = std::function<void(nlohmann::json)>;
    void SetJSONCallback(JSONCallback_t func);
    void Send(const std::string &str);

private:
    void OnMessage(const ix::WebSocketMessagePtr &msg);

    JSONCallback_t m_json_callback;
    ix::WebSocket m_websocket;
};
