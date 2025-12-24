#pragma once
#ifdef WITH_VOICE
// clang-format off

#include "snowflake.hpp"
#include "waiter.hpp"
#include "websocket.hpp"
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <atomic>
#include <glibmm/dispatcher.h>
#include <sigc++/sigc++.h>
#include <spdlog/logger.h>
#include <unordered_map>
#ifdef WITH_VIDEO
#include "video/capture.hpp"
#include "discord/rtppacketizer.hpp"
#endif
// clang-format on

enum class VoiceGatewayCloseCode : uint16_t {
    Normal = 4000,
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
    SessionUpdate = 14,
    MediaSinkWants = 15,
    VoiceBackendVersion = 16,
    ChannelOptionsUpdate = 17,
    Flags = 18,
    SpeedTest = 19,
    Platform = 20,
    SecureFramesPrepareProtocolTransition = 21,
    SecureFramesExecuteTransition = 22,
    SecureFramesReadyForTransition = 23,
    SecureFramesPrepareEpoch = 24,
    MlsExternalSenderPackage = 25,
    MlsKeyPackage = 26,
    MlsProposals = 27,
    MlsCommitWelcome = 28,
    MlsPrepareCommitTransition = 29,
    MlsWelcome = 30,
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
    bool IsScreenShare = false;
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
    std::optional<nlohmann::json> Codecs;

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

struct VoiceVideoSourceUpdateMessage {
    uint32_t AudioSSRC;
    uint32_t VideoSSRC;
    uint32_t RTXSSRC;
    struct Stream {
        std::string Type;
        std::string RID;
        uint32_t SSRC;
        bool Active;
        int Quality;
        uint32_t RTXSSRC;
        int MaxBitrate;
        int MaxFramerate;
        struct Resolution {
            int Width;
            int Height;
        } MaxResolution;

        friend void to_json(nlohmann::json &j, const Stream &s);
    };
    std::vector<Stream> Streams;

    friend void to_json(nlohmann::json &j, const VoiceVideoSourceUpdateMessage &m);
};

enum class VoiceSpeakingType {
    Microphone = 1 << 0,
    Soundshare = 1 << 1,
    Priority = 1 << 2,
};

struct VoiceSpeakingMessage {
    VoiceSpeakingType Speaking;
    int Delay;
    uint32_t SSRC;

    friend void to_json(nlohmann::json &j, const VoiceSpeakingMessage &m);
};

struct VoiceSpeakingData {
    Snowflake UserID;
    uint32_t SSRC;
    VoiceSpeakingType Speaking;

    friend void from_json(const nlohmann::json &j, VoiceSpeakingData &m);
};

class UDPSocket {
public:
    UDPSocket();
    ~UDPSocket();

    void Connect(std::string_view ip, uint16_t port);
    void Run();
    void SetSecretKey(std::array<uint8_t, 32> key);
    void SetSSRC(uint32_t ssrc);
    void SetEncryptionMode(const std::string &mode);
    void SendEncrypted(const uint8_t *data, size_t len, uint32_t ssrc = 0);
    void SendEncrypted(const std::vector<uint8_t> &data, uint32_t ssrc = 0);
    void SendEncryptedRTP(const uint8_t *rtp_packet, size_t len);
    void SendEncryptedRTP(const std::vector<uint8_t> &rtp_packet);
    void Send(const uint8_t *data, size_t len);
    std::vector<uint8_t> Receive();
    void SetReceiveTimeout(int timeout_ms);
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
    std::string m_encryption_mode;
    std::atomic<uint32_t> m_packet_counter{0};

    std::atomic<uint16_t> m_sequence{0};
    std::atomic<uint64_t> m_nonce_counter{0};

public:
    using type_signal_data = sigc::signal<void, std::vector<uint8_t>>;
    type_signal_data signal_data();

private:
    type_signal_data m_signal_data;
};

class DiscordClient; // Forward declaration

class DiscordVoiceClient {
    friend class DiscordClient;

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

    void SetStreamKey(std::string_view key);
    void SetStreamToken(std::string_view token);
    void SetStreamEndpoint(std::string_view endpoint);
    void SetStreamPaused(bool paused) noexcept;
    [[nodiscard]] bool IsStreamPaused() const noexcept;
    void SetScreenShareMode(bool is_screenshare) noexcept;
    [[nodiscard]] bool IsScreenShareMode() const noexcept;
    [[nodiscard]] bool IsIdentifiedAsScreenShare() const noexcept;
    void Reconnect();

    // todo serialize
    void SetUserVolume(Snowflake id, float volume);
    [[nodiscard]] float GetUserVolume(Snowflake id) const;
    [[nodiscard]] std::optional<uint32_t> GetSSRCOfUser(Snowflake id) const;

    void SetVideoStatus(bool active);
    [[nodiscard]] bool IsVideoActive() const noexcept;

    // Is a websocket and udp connection fully established
    [[nodiscard]] bool IsConnected() const noexcept;
    [[nodiscard]] bool IsConnecting() const noexcept;

    enum class State {
        ConnectingToWebsocket,
        EstablishingConnection,
        Connected,
        DisconnectedByClient,
        DisconnectedByServer,
    };

private:
    static const char *GetStateName(State state);

    void OnGatewayMessage(const std::string &msg);
    void HandleGatewayHello(const VoiceGatewayMessage &m);
    void HandleGatewayReady(const VoiceGatewayMessage &m);
    void HandleGatewaySessionDescription(const VoiceGatewayMessage &m);
    void HandleGatewaySpeaking(const VoiceGatewayMessage &m);

    void Identify();
    void Discovery();
    void SelectProtocol(const char *ip, uint16_t port);

    void OnWebsocketOpen();
    void OnWebsocketClose(const ix::WebSocketCloseInfo &info);
    void OnWebsocketMessage(const std::string &str);

    void HeartbeatThread();
    void KeepaliveThread();

    void SetState(State state);

    void OnUDPData(std::vector<uint8_t> data);

    std::string m_session_id;
    std::string m_endpoint;
    std::string m_token;
    std::string m_stream_key;
    std::string m_stream_token;
    std::string m_stream_endpoint;
    std::atomic<bool> m_stream_paused = false;
    std::atomic<bool> m_is_screenshare = false;
    std::atomic<bool> m_identified_as_screenshare = false;
    Snowflake m_server_id;
    Snowflake m_channel_id;
    Snowflake m_user_id;

    std::unordered_map<Snowflake, uint32_t> m_ssrc_map;
    std::unordered_map<Snowflake, float> m_user_volumes;

    std::array<uint8_t, 32> m_secret_key;

    std::string m_ip;
    uint16_t m_port;
    uint32_t m_ssrc;

    uint32_t m_video_ssrc = 0;
    uint32_t m_rtx_ssrc = 0;
    std::atomic<bool> m_video_active{false};
    std::string m_encryption_mode;

    int m_heartbeat_msec;
    Waiter m_heartbeat_waiter;
    std::thread m_heartbeat_thread;

    Waiter m_keepalive_waiter;
    std::thread m_keepalive_thread;

    Websocket m_ws;
    UDPSocket m_udp;

    Glib::Dispatcher m_dispatcher;
    std::queue<std::string> m_dispatch_queue;
    std::mutex m_dispatch_mutex;

    void OnDispatch();

    std::array<uint8_t, 1275> m_opus_buffer;

#ifdef WITH_VIDEO
    std::unique_ptr<VideoCapture> m_video_capture;
    sigc::connection m_video_packet_connection;
    std::atomic<uint32_t> m_video_timestamp{0};
    std::atomic<uint16_t> m_video_sequence{0};
#endif

    std::shared_ptr<spdlog::logger> m_log;

    std::atomic<State> m_state;

    using type_signal_connected = sigc::signal<void()>;
    using type_signal_disconnected = sigc::signal<void()>;
    using type_signal_speaking = sigc::signal<void(VoiceSpeakingData)>;
    using type_signal_state_update = sigc::signal<void(State)>;
    using type_signal_video_state_changed = sigc::signal<void(bool active)>;
    using type_signal_pli_received = sigc::signal<void(uint32_t ssrc)>;
    using type_signal_fir_received = sigc::signal<void(uint32_t ssrc)>;
    type_signal_connected m_signal_connected;
    type_signal_disconnected m_signal_disconnected;
    type_signal_speaking m_signal_speaking;
    type_signal_state_update m_signal_state_update;
    type_signal_video_state_changed m_signal_video_state_changed;
    type_signal_pli_received m_signal_pli_received;
    type_signal_fir_received m_signal_fir_received;

public:
    type_signal_connected signal_connected();
    type_signal_disconnected signal_disconnected();
    type_signal_speaking signal_speaking();
    type_signal_state_update signal_state_update();
    type_signal_video_state_changed signal_video_state_changed();
    type_signal_pli_received signal_pli_received();
    type_signal_fir_received signal_fir_received();
};
#endif
