#pragma once
#include <expected>
#include <string>
#include <glibmm.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <curl/curl.h>

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

    enum class Error {
        recv_error,
        select_error,
        closing,
    };

    enum class State {
        Closed,
        Connecting,
        Open,
        Closing,
    };

    struct CloseInfo {
        bool Remote;
        uint16_t Code;
        std::string Reason;
    };

private:
    struct WebSocketMessage {
        enum class MessageType {
            ping,
            text,
            binary,
            close,
        };

        CloseInfo CloseInfo {};

        std::vector<uint8_t> Data;
        MessageType Type;
    };

    State m_state = State::Closed;

    void OnMessage(const WebSocketMessage &message);
    std::expected<WebSocketMessage, Error> ReceiveMessage();
    void Task();

    mutable std::mutex m_mutex;
    std::thread m_thread;
    CURL *m_websocket = nullptr;

    std::atomic<bool> m_sent_close = false;
    mutable std::mutex m_close_mutex;
    std::condition_variable m_close_cv;

public:
    using type_signal_open = sigc::signal<void>;
    using type_signal_close = sigc::signal<void, CloseInfo>;
    using type_signal_message = sigc::signal<void, std::string>;

    type_signal_open signal_open();
    type_signal_close signal_close();
    type_signal_message signal_message();

private:
    type_signal_open m_signal_open;
    type_signal_close m_signal_close;
    type_signal_message m_signal_message;

    bool m_print_messages = true;

    CloseInfo m_close_info;

    Glib::Dispatcher m_open_dispatcher;
    Glib::Dispatcher m_close_dispatcher;

    std::shared_ptr<spdlog::logger> m_log;
};
