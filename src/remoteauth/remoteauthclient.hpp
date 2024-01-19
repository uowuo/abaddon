#pragma once

#ifdef WITH_QRLOGIN

// clang-format off

#include <optional>
#include <string>
#include <queue>

#include <spdlog/logger.h>

#include "discord/errors.hpp"
#include "discord/snowflake.hpp"
#include "discord/waiter.hpp"
#include "discord/websocket.hpp"
#include "ssl.hpp"

// clang-format on

class RemoteAuthClient {
public:
    RemoteAuthClient();
    ~RemoteAuthClient();

    void Start();
    void Stop();

    [[nodiscard]] bool IsConnected() const noexcept;

private:
    void OnGatewayMessage(const std::string &str);
    void HandleGatewayHello(const nlohmann::json &j);
    void HandleGatewayNonceProof(const nlohmann::json &j);
    void HandleGatewayPendingRemoteInit(const nlohmann::json &j);
    void HandleGatewayPendingTicket(const nlohmann::json &j);
    void HandleGatewayPendingLogin(const nlohmann::json &j);
    void HandleGatewayCancel(const nlohmann::json &j);

    void OnRemoteAuthLoginResponse(const std::optional<std::string> &encrypted_token, DiscordError err);

    void Init();

    void GenerateKey();
    std::string GetEncodedPublicKey() const;

    std::vector<uint8_t> Decrypt(const unsigned char *in, size_t inlen) const;

    void OnWebsocketOpen();
    void OnWebsocketClose(const ix::WebSocketCloseInfo &info);
    void OnWebsocketMessage(const std::string &str);

    void HeartbeatThread();

    int m_heartbeat_msec;
    Waiter m_heartbeat_waiter;
    std::thread m_heartbeat_thread;

    Glib::Dispatcher m_dispatcher;
    std::queue<std::string> m_dispatch_queue;
    std::mutex m_dispatch_mutex;

    void OnDispatch();

    bool OnTimeout();
    sigc::connection m_timeout_conn;

    Websocket m_ws;
    bool m_connected = false;

    std::shared_ptr<spdlog::logger> m_log;

    EVP_PKEY_CTX_ptr m_pkey_ctx;
    EVP_PKEY_CTX_ptr m_dec_ctx;
    EVP_PKEY_ptr m_pkey;

public:
    using type_signal_hello = sigc::signal<void()>;
    using type_signal_fingerprint = sigc::signal<void(std::string)>;
    using type_signal_pending_ticket = sigc::signal<void(Snowflake, std::string, std::string, std::string)>;
    using type_signal_pending_login = sigc::signal<void()>;
    using type_signal_token = sigc::signal<void(std::string)>;
    using type_signal_error = sigc::signal<void(std::string)>;
    type_signal_hello signal_hello();
    type_signal_fingerprint signal_fingerprint();
    type_signal_pending_ticket signal_pending_ticket();
    type_signal_pending_login signal_pending_login();
    type_signal_token signal_token();
    type_signal_error signal_error();

private:
    type_signal_hello m_signal_hello;
    type_signal_fingerprint m_signal_fingerprint;
    type_signal_pending_ticket m_signal_pending_ticket;
    type_signal_pending_login m_signal_pending_login;
    type_signal_token m_signal_token;
    type_signal_error m_signal_error;
};

#endif
