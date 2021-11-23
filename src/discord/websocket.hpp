#pragma once
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include <sigc++/sigc++.h>

class Websocket {
public:
    Websocket();
    void StartConnection(std::string url);

    void SetUserAgent(std::string agent);

    void Send(const std::string &str);
    void Send(const nlohmann::json &j);
    void Stop();
    void Stop(uint16_t code);
    bool IsOpen() const;

private:
    void OnMessage(const ix::WebSocketMessagePtr &msg);

    ix::WebSocket m_websocket;
    std::string m_agent;

public:
    typedef sigc::signal<void> type_signal_open;
    typedef sigc::signal<void, uint16_t> type_signal_close;
    typedef sigc::signal<void, std::string> type_signal_message;

    type_signal_open signal_open();
    type_signal_close signal_close();
    type_signal_message signal_message();

private:
    type_signal_open m_signal_open;
    type_signal_close m_signal_close;
    type_signal_message m_signal_message;
};
