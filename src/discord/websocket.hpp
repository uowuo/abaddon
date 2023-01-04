#pragma once
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <string>
#include <functional>
#include <glibmm.h>
#include <nlohmann/json.hpp>
#include <sigc++/sigc++.h>
#include <spdlog/spdlog.h>

class Websocket {
public:
    Websocket(const std::string &id);
    void StartConnection(const std::string &url);

    void SetUserAgent(std::string agent);

    bool GetPrintMessages() const noexcept;
    void SetPrintMessages(bool show) noexcept;

    void Send(const std::string &str);
    void Send(const nlohmann::json &j);
    void Stop();
    void Stop(uint16_t code);

private:
    void OnMessage(const ix::WebSocketMessagePtr &msg);

    std::unique_ptr<ix::WebSocket> m_websocket;
    std::string m_agent;

public:
    using type_signal_open = sigc::signal<void>;
    using type_signal_close = sigc::signal<void, ix::WebSocketCloseInfo>;
    using type_signal_message = sigc::signal<void, std::string>;

    type_signal_open signal_open();
    type_signal_close signal_close();
    type_signal_message signal_message();

private:
    type_signal_open m_signal_open;
    type_signal_close m_signal_close;
    type_signal_message m_signal_message;

    bool m_print_messages = true;

    Glib::Dispatcher m_open_dispatcher;
    Glib::Dispatcher m_close_dispatcher;
    ix::WebSocketCloseInfo m_close_info;

    std::shared_ptr<spdlog::logger> m_log;
};
