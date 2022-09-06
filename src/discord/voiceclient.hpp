#pragma once
#ifdef WITH_VOICE
// clang-format off
#include "snowflake.hpp"
#include "waiter.hpp"
#include "websocket.hpp"
#include <mutex>
#include <queue>
#include <string>
#include <glibmm/dispatcher.h>
// clang-format on

enum class VoiceGatewayCloseCode : uint16_t {
    UnknownOpcode = 4001,
    InvalidPayload = 4002,
    NotAuthenticated = 4003,
    AuthenticationFailed = 4004,
    AlreadyAuthenticated = 4005,
    SessionInvalid = 4006,
    SessionTimedOut = 4009,
    ServerNotFound = 4011,
    UnknownProtocol = 4012,
    Disconnected = 4014,
    ServerCrashed = 4015,
    UnknownEncryption = 4016,
};

enum class VoiceGatewayOp : int {
    Identify = 0,
    SelectProtocol = 1,
    Ready = 2,
    Heartbeat = 3,
    SessionDescription = 4,
    Speaking = 5,
    HeartbeatAck = 6,
    Resume = 7,
    Hello = 8,
    Resumed = 9,
    ClientDisconnect = 13,
};

struct VoiceGatewayMessage {
    VoiceGatewayOp Opcode;
    nlohmann::json Data;

    friend void from_json(const nlohmann::json &j, VoiceGatewayMessage &m);
};

struct VoiceHelloData {
    int HeartbeatInterval;

    friend void from_json(const nlohmann::json &j, VoiceHelloData &m);
};

struct VoiceHeartbeatMessage {
    uint64_t Nonce;

    friend void to_json(nlohmann::json &j, const VoiceHeartbeatMessage &m);
};

struct VoiceIdentifyMessage {
    Snowflake ServerID;
    Snowflake UserID;
    std::string SessionID;
    std::string Token;
    bool Video;
    // todo streams i guess?

    friend void to_json(nlohmann::json &j, const VoiceIdentifyMessage &m);
};

struct VoiceReadyData {
    struct VoiceStream {
        bool IsActive;
        int Quality;
        std::string RID;
        int RTXSSRC;
        int SSRC;
        std::string Type;

        friend void from_json(const nlohmann::json &j, VoiceStream &m);
    };

    std::vector<std::string> Experiments;
    std::string IP;
    std::vector<std::string> Modes;
    uint16_t Port;
    uint32_t SSRC;
    std::vector<VoiceStream> Streams;

    friend void from_json(const nlohmann::json &j, VoiceReadyData &m);
};

struct VoiceSelectProtocolMessage {
    std::string Address;
    uint16_t Port;
    std::string Mode;
    std::string Protocol;

    friend void to_json(nlohmann::json &j, const VoiceSelectProtocolMessage &m);
};

struct VoiceSessionDescriptionData {
    // std::string AudioCodec;
    // std::string VideoCodec;
    // std::string MediaSessionID;
    std::string Mode;
    std::array<uint8_t, 32> SecretKey;

    friend void from_json(const nlohmann::json &j, VoiceSessionDescriptionData &m);
};

struct VoiceSpeakingMessage {
    enum {
        Microphone = 1 << 0,
        Soundshare = 1 << 1,
        Priority = 1 << 2,
    };

    int Speaking;
    int Delay;
    uint32_t SSRC;

    friend void to_json(nlohmann::json &j, const VoiceSpeakingMessage &m);
};

class UDPSocket {
public:
    UDPSocket();
    ~UDPSocket();

    void Connect(std::string_view ip, uint16_t port);
    void Run();
    void SetSecretKey(std::array<uint8_t, 32> key);
    void SetSSRC(uint32_t ssrc);
    void SendEncrypted(const uint8_t *data, size_t len);
    void SendEncrypted(const std::vector<uint8_t> &data);
    void Send(const uint8_t *data, size_t len);
    std::vector<uint8_t> Receive();
    void Stop();

private:
    void ReadThread();

    #ifdef _WIN32
    SOCKET m_socket;
    #else
    int m_socket;
    #endif
    sockaddr_in m_server;

    std::atomic<bool> m_running = false;

    std::thread m_thread;

    std::array<uint8_t, 32> m_secret_key;
    uint32_t m_ssrc;

    uint16_t m_sequence = 0;
    uint32_t m_timestamp = 0;

public:
    using type_signal_data = sigc::signal<void, std::vector<uint8_t>>;
    type_signal_data signal_data();

private:
    type_signal_data m_signal_data;
};

class DiscordVoiceClient {
public:
    DiscordVoiceClient();
    ~DiscordVoiceClient();

    void Start();
    void Stop();

    void SetSessionID(std::string_view session_id);
    void SetEndpoint(std::string_view endpoint);
    void SetToken(std::string_view token);
    void SetServerID(Snowflake id);
    void SetUserID(Snowflake id);

    [[nodiscard]] bool IsConnected() const noexcept;

private:
    void OnGatewayMessage(const std::string &str);
    void HandleGatewayHello(const VoiceGatewayMessage &m);
    void HandleGatewayReady(const VoiceGatewayMessage &m);
    void HandleGatewaySessionDescription(const VoiceGatewayMessage &m);

    void Identify();
    void Discovery();
    void SelectProtocol(std::string_view ip, uint16_t port);

    void OnUDPData(std::vector<uint8_t> data);

    void HeartbeatThread();

    std::string m_session_id;
    std::string m_endpoint;
    std::string m_token;
    Snowflake m_server_id;
    Snowflake m_channel_id;
    Snowflake m_user_id;

    std::string m_ip;
    uint16_t m_port;
    uint32_t m_ssrc;

    std::array<uint8_t, 32> m_secret_key;

    Websocket m_ws;
    UDPSocket m_udp;

    Glib::Dispatcher m_dispatcher;
    std::queue<std::string> m_message_queue;
    std::mutex m_dispatch_mutex;

    Glib::Dispatcher m_udp_dispatcher;
    std::queue<std::vector<uint8_t>> m_udp_message_queue;
    std::mutex m_udp_dispatch_mutex;

    int m_heartbeat_msec;
    Waiter m_heartbeat_waiter;
    std::thread m_heartbeat_thread;

    std::array<uint8_t, 1275> m_opus_buffer;

    std::atomic<bool> m_connected = false;
};
#endif
