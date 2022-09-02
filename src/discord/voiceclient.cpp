#include "voiceclient.hpp"
#include "json.hpp"
#include <sodium.h>
#include "abaddon.hpp"
#include "audio/manager.hpp"

#ifdef _WIN32
    #define S_ADDR(var) (var).sin_addr.S_un.S_addr
    #define socklen_t int
#else
    #define S_ADDR(var) (var).sin_addr.s_addr
#endif

UDPSocket::UDPSocket() {
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
}

UDPSocket::~UDPSocket() {
    Stop();
}

void UDPSocket::Connect(std::string_view ip, uint16_t port) {
    std::memset(&m_server, 0, sizeof(m_server));
    m_server.sin_family = AF_INET;
    S_ADDR(m_server) = inet_addr(ip.data());
    m_server.sin_port = htons(port);
    bind(m_socket, reinterpret_cast<sockaddr *>(&m_server), sizeof(m_server));
}

void UDPSocket::Run() {
    m_running = true;
    m_thread = std::thread(&UDPSocket::ReadThread, this);
}

void UDPSocket::SetSecretKey(std::array<uint8_t, 32> key) {
    m_secret_key = key;
}

void UDPSocket::SetSSRC(uint32_t ssrc) {
    m_ssrc = ssrc;
}

void UDPSocket::SendEncrypted(const std::vector<uint8_t> &data) {
    m_sequence++;
    m_timestamp += (48000 / 100) * 2;

    std::vector<uint8_t> rtp(12, 0);
    rtp[0] = 0x80; // ver 2
    rtp[1] = 0x78; // payload type 0x78
    rtp[2] = (m_sequence >> 8) & 0xFF;
    rtp[3] = (m_sequence >> 0) & 0xFF;
    rtp[4] = (m_timestamp >> 24) & 0xFF;
    rtp[5] = (m_timestamp >> 16) & 0xFF;
    rtp[6] = (m_timestamp >> 8) & 0xFF;
    rtp[7] = (m_timestamp >> 0) & 0xFF;
    rtp[8] = (m_ssrc >> 24) & 0xFF;
    rtp[9] = (m_ssrc >> 16) & 0xFF;
    rtp[10] = (m_ssrc >> 8) & 0xFF;
    rtp[11] = (m_ssrc >> 0) & 0xFF;

    static std::array<uint8_t, 24> nonce = {};
    std::memcpy(nonce.data(), rtp.data(), 12);

    std::vector<uint8_t> ciphertext(crypto_secretbox_MACBYTES + rtp.size(), 0);
    crypto_secretbox_easy(ciphertext.data(), rtp.data(), rtp.size(), nonce.data(), m_secret_key.data());
    rtp.insert(rtp.end(), ciphertext.begin(), ciphertext.end());

    Send(rtp.data(), rtp.size());
}

void UDPSocket::Send(const uint8_t *data, size_t len) {
    sendto(m_socket, reinterpret_cast<const char *>(data), static_cast<int>(len), 0, reinterpret_cast<sockaddr *>(&m_server), sizeof(m_server));
}

std::vector<uint8_t> UDPSocket::Receive() {
    while (true) {
        sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        static std::array<uint8_t, 4096> buf;
        int n = recvfrom(m_socket, reinterpret_cast<char *>(buf.data()), sizeof(buf), 0, reinterpret_cast<sockaddr *>(&from), &fromlen);
        if (n < 0) {
            return {};
        } else if (S_ADDR(from) == S_ADDR(m_server) && from.sin_port == m_server.sin_port) {
            return { buf.begin(), buf.begin() + n };
        }
    }
}

void UDPSocket::Stop() {
    m_running = false;
#ifdef _WIN32
    shutdown(m_socket, SD_BOTH);
#else
    shutdown(m_socket, SHUT_RDWR);
#endif
    if (m_thread.joinable()) m_thread.join();
}

void UDPSocket::ReadThread() {
    while (m_running) {
        static std::array<uint8_t, 4096> buf;
        sockaddr_in from;
        socklen_t addrlen = sizeof(from);
        int n = recvfrom(m_socket, reinterpret_cast<char *>(buf.data()), sizeof(buf), 0, reinterpret_cast<sockaddr *>(&from), &addrlen);
        if (n > 0 && S_ADDR(from) == S_ADDR(m_server) && from.sin_port == m_server.sin_port) {
            m_signal_data.emit({ buf.begin(), buf.begin() + n });
        }
    }
}

UDPSocket::type_signal_data UDPSocket::signal_data() {
    return m_signal_data;
}

DiscordVoiceClient::DiscordVoiceClient() {
    sodium_init();

    m_ws.signal_open().connect([this]() {
        puts("vws open");
    });

    m_ws.signal_close().connect([this](uint16_t code) {
        printf("vws close %u\n", code);
    });

    m_ws.signal_message().connect([this](const std::string &str) {
        std::lock_guard<std::mutex> _(m_dispatch_mutex);
        m_message_queue.push(str);
        m_dispatcher.emit();
    });

    m_udp.signal_data().connect([this](const std::vector<uint8_t> &data) {
        std::lock_guard<std::mutex> _(m_udp_dispatch_mutex);
        m_udp_message_queue.push(data);
        m_udp_dispatcher.emit();
    });

    m_dispatcher.connect([this]() {
        m_dispatch_mutex.lock();
        if (m_message_queue.empty()) {
            m_dispatch_mutex.unlock();
            return;
        }
        auto msg = std::move(m_message_queue.front());
        m_message_queue.pop();
        m_dispatch_mutex.unlock();
        OnGatewayMessage(msg);
    });

    m_udp_dispatcher.connect([this]() {
        m_udp_dispatch_mutex.lock();
        if (m_udp_message_queue.empty()) {
            m_udp_dispatch_mutex.unlock();
            return;
        }
        auto data = std::move(m_udp_message_queue.front());
        m_udp_message_queue.pop();
        m_udp_dispatch_mutex.unlock();
        OnUDPData(data);
    });
}

DiscordVoiceClient::~DiscordVoiceClient() {
    m_ws.Stop();
    m_udp.Stop();
    m_heartbeat_waiter.kill();
    if (m_heartbeat_thread.joinable()) m_heartbeat_thread.join();
}

void DiscordVoiceClient::Start() {
    m_ws.StartConnection("wss://" + m_endpoint + "/?v=7");
}

void DiscordVoiceClient::SetSessionID(std::string_view session_id) {
    m_session_id = session_id;
}

void DiscordVoiceClient::SetEndpoint(std::string_view endpoint) {
    m_endpoint = endpoint;
}

void DiscordVoiceClient::SetToken(std::string_view token) {
    m_token = token;
}

void DiscordVoiceClient::SetServerID(Snowflake id) {
    m_server_id = id;
}

void DiscordVoiceClient::SetUserID(Snowflake id) {
    m_user_id = id;
}

void DiscordVoiceClient::OnGatewayMessage(const std::string &str) {
    VoiceGatewayMessage msg = nlohmann::json::parse(str);
    puts(msg.Data.dump(4).c_str());
    switch (msg.Opcode) {
        case VoiceGatewayOp::Hello: {
            HandleGatewayHello(msg);
        } break;
        case VoiceGatewayOp::Ready: {
            HandleGatewayReady(msg);
        } break;
        case VoiceGatewayOp::SessionDescription: {
            HandleGatewaySessionDescription(msg);
        } break;
        default: break;
    }
}

void DiscordVoiceClient::HandleGatewayHello(const VoiceGatewayMessage &m) {
    VoiceHelloData d = m.Data;
    m_heartbeat_msec = d.HeartbeatInterval;
    m_heartbeat_thread = std::thread(&DiscordVoiceClient::HeartbeatThread, this);

    Identify();
}

void DiscordVoiceClient::HandleGatewayReady(const VoiceGatewayMessage &m) {
    VoiceReadyData d = m.Data;
    m_ip = d.IP;
    m_port = d.Port;
    m_ssrc = d.SSRC;
    if (std::find(d.Modes.begin(), d.Modes.end(), "xsalsa20_poly1305") == d.Modes.end()) {
        puts("xsalsa20_poly1305 not in encryption modes");
    }
    printf("connect to %s:%u ssrc %u\n", m_ip.c_str(), m_port, m_ssrc);

    m_udp.Connect(m_ip, m_port);

    Discovery();
}

void DiscordVoiceClient::HandleGatewaySessionDescription(const VoiceGatewayMessage &m) {
    VoiceSessionDescriptionData d = m.Data;
    printf("receiving with %s secret key: ", d.Mode.c_str());
    for (auto b : d.SecretKey) {
        printf("%02X", b);
    }
    printf("\n");
    m_secret_key = d.SecretKey;
    m_udp.SetSSRC(m_ssrc);
    m_udp.SetSecretKey(m_secret_key);
    m_udp.SendEncrypted({ 0xF8, 0xFF, 0xFE });
    m_udp.SendEncrypted({ 0xF8, 0xFF, 0xFE });
    m_udp.SendEncrypted({ 0xF8, 0xFF, 0xFE });
    m_udp.SendEncrypted({ 0xF8, 0xFF, 0xFE });
    m_udp.SendEncrypted({ 0xF8, 0xFF, 0xFE });
    m_udp.Run();
}

void DiscordVoiceClient::Identify() {
    VoiceIdentifyMessage msg;
    msg.ServerID = m_server_id;
    msg.UserID = m_user_id;
    msg.SessionID = m_session_id;
    msg.Token = m_token;
    msg.Video = true;
    m_ws.Send(msg);
}

void DiscordVoiceClient::Discovery() {
    std::vector<uint8_t> payload;
    // 2 bytes = 1, request
    payload.push_back(0x00);
    payload.push_back(0x01);
    // 2 bytes = 70, pl length
    payload.push_back(0x00);
    payload.push_back(70);
    // 4 bytes = ssrc
    payload.push_back((m_ssrc >> 24) & 0xFF);
    payload.push_back((m_ssrc >> 16) & 0xFF);
    payload.push_back((m_ssrc >> 8) & 0xFF);
    payload.push_back((m_ssrc >> 0) & 0xFF);
    // address and port
    for (int i = 0; i < 66; i++)
        payload.push_back(0);
    m_udp.Send(payload.data(), payload.size());
    auto response = m_udp.Receive();
    if (response.size() >= 74 && response[0] == 0x00 && response[1] == 0x02) {
        const char *our_ip = reinterpret_cast<const char *>(&response[8]);
        uint16_t our_port = (response[73] << 8) | response[74];
        printf("we are %s:%u\n", our_ip, our_port);
        SelectProtocol(our_ip, our_port);
    } else {
        puts("received non-discovery packet after discovery");
    }
}

void DiscordVoiceClient::SelectProtocol(std::string_view ip, uint16_t port) {
    VoiceSelectProtocolMessage msg;
    msg.Mode = "xsalsa20_poly1305";
    msg.Address = ip;
    msg.Port = port;
    msg.Protocol = "udp";
    m_ws.Send(msg);
}

void DiscordVoiceClient::OnUDPData(std::vector<uint8_t> data) {
    uint8_t *payload = data.data() + 12;
    uint32_t ssrc = (data[8] << 24) |
                    (data[9] << 16) |
                    (data[10] << 8) |
                    (data[11] << 0);
    static std::array<uint8_t, 24> nonce = {};
    std::memcpy(nonce.data(), data.data(), 12);
    if (crypto_secretbox_open_easy(payload, payload, data.size() - 12, nonce.data(), m_secret_key.data())) {
        puts("decrypt fail");
    } else {
        Abaddon::Get().GetAudio().FeedMeOpus(ssrc, { payload, payload + data.size() - 12 - crypto_box_MACBYTES });
    }
}

void DiscordVoiceClient::HeartbeatThread() {
    while (true) {
        if (!m_heartbeat_waiter.wait_for(std::chrono::milliseconds(m_heartbeat_msec)))
            break;

        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();

        VoiceHeartbeatMessage msg;
        msg.Nonce = static_cast<uint64_t>(ms);
        m_ws.Send(msg);
    }
}

void from_json(const nlohmann::json &j, VoiceGatewayMessage &m) {
    JS_D("op", m.Opcode);
    m.Data = j.at("d");
}

void from_json(const nlohmann::json &j, VoiceHelloData &m) {
    JS_D("heartbeat_interval", m.HeartbeatInterval);
}

void to_json(nlohmann::json &j, const VoiceHeartbeatMessage &m) {
    j["op"] = VoiceGatewayOp::Heartbeat;
    j["d"] = m.Nonce;
}

void to_json(nlohmann::json &j, const VoiceIdentifyMessage &m) {
    j["op"] = VoiceGatewayOp::Identify;
    j["d"]["server_id"] = m.ServerID;
    j["d"]["user_id"] = m.UserID;
    j["d"]["session_id"] = m.SessionID;
    j["d"]["token"] = m.Token;
    j["d"]["video"] = m.Video;
    j["d"]["streams"][0]["type"] = "video";
    j["d"]["streams"][0]["rid"] = "100";
    j["d"]["streams"][0]["quality"] = 100;
}

void from_json(const nlohmann::json &j, VoiceReadyData::VoiceStream &m) {
    JS_D("active", m.IsActive);
    JS_D("quality", m.Quality);
    JS_D("rid", m.RID);
    JS_D("rtx_ssrc", m.RTXSSRC);
    JS_D("ssrc", m.SSRC);
    JS_D("type", m.Type);
}

void from_json(const nlohmann::json &j, VoiceReadyData &m) {
    JS_ON("experiments", m.Experiments);
    JS_D("ip", m.IP);
    JS_D("modes", m.Modes);
    JS_D("port", m.Port);
    JS_D("ssrc", m.SSRC);
    JS_ON("streams", m.Streams);
}

void to_json(nlohmann::json &j, const VoiceSelectProtocolMessage &m) {
    j["op"] = VoiceGatewayOp::SelectProtocol;
    j["d"]["address"] = m.Address;
    j["d"]["port"] = m.Port;
    j["d"]["protocol"] = m.Protocol;
    j["d"]["mode"] = m.Mode;
    j["d"]["data"]["address"] = m.Address;
    j["d"]["data"]["port"] = m.Port;
    j["d"]["data"]["mode"] = m.Mode;
}

void from_json(const nlohmann::json &j, VoiceSessionDescriptionData &m) {
    JS_D("mode", m.Mode);
    JS_D("secret_key", m.SecretKey);
}
