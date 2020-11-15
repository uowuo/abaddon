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

    using MessageCallback_t = std::function<void(std::string data)>;
    void SetMessageCallback(MessageCallback_t func);
    void Send(const std::string &str);
    void Send(const nlohmann::json &j);
    void Stop();
    void Stop(uint16_t code);
    bool IsOpen() const;

private:
    void OnMessage(const ix::WebSocketMessagePtr &msg);

    MessageCallback_t m_callback;
    ix::WebSocket m_websocket;
};
