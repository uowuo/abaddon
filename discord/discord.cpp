#include "../abaddon.hpp"
#include "discord.hpp"
#include <cassert>

DiscordClient::DiscordClient() {
    LoadEventMap();
}

void DiscordClient::SetAbaddon(Abaddon *ptr) {
    m_abaddon = ptr;
}

void DiscordClient::Start() {
    assert(!m_client_connected);
    assert(!m_websocket.IsOpen());

    m_client_connected = true;
    m_websocket.StartConnection(DiscordGateway);
    m_websocket.SetJSONCallback(std::bind(&DiscordClient::HandleGatewayMessage, this, std::placeholders::_1));
}

void DiscordClient::Stop() {
    if (!m_client_connected) return;

    m_heartbeat_waiter.kill();
    m_heartbeat_thread.join();
    m_client_connected = false;
    m_websocket.Stop();
}

bool DiscordClient::IsStarted() const {
    return m_client_connected;
}

void DiscordClient::HandleGatewayMessage(nlohmann::json j) {
    GatewayMessage m;
    try {
        m = j;
    } catch (std::exception &e) {
        printf("Error decoding JSON. Discarding message: %s\n", e.what());
        return;
    }

    switch (m.Opcode) {
        case GatewayOp::Hello: {
            HelloMessageData d = m.Data;
            m_heartbeat_msec = d.HeartbeatInterval;
            m_heartbeat_thread = std::thread(std::bind(&DiscordClient::HeartbeatThread, this));
            SendIdentify();
        } break;
        case GatewayOp::HeartbeatAck: {
            m_heartbeat_acked = true;
        } break;
        case GatewayOp::Event: {
            auto iter = m_event_map.find(m.Type);
            if (iter == m_event_map.end()) {
                printf("Unknown event %s\n", m.Type.c_str());
                break;
            }
            switch (iter->second) {
                case GatewayEvent::READY: {
                    HandleGatewayReady(m);
                }
            }
        } break;
        default:
            printf("Unknown opcode %d\n", m.Opcode);
            break;
    }
}

void DiscordClient::HandleGatewayReady(const GatewayMessage &msg) {

}

void DiscordClient::HeartbeatThread() {
    while (m_client_connected) {
        if (!m_heartbeat_acked) {
            printf("wow! a heartbeat wasn't acked! how could this happen?");
        }

        m_heartbeat_acked = false;

        HeartbeatMessage msg;
        msg.Sequence = m_last_sequence;
        nlohmann::json j = msg;
        m_websocket.Send(j);

        if (!m_heartbeat_waiter.wait_for(std::chrono::milliseconds(m_heartbeat_msec)))
            break;
    }
}

void DiscordClient::SendIdentify() {
    auto token = m_abaddon->GetDiscordToken();
    assert(token.size());
    IdentifyMessage msg;
    msg.Properties.OS = "OpenBSD";
    msg.Properties.Device = GatewayIdentity;
    msg.Properties.Browser = GatewayIdentity;
    msg.Token = token;
    m_websocket.Send(msg);
}

void DiscordClient::LoadEventMap() {
    m_event_map["READY"] = GatewayEvent::READY;
}

void from_json(const nlohmann::json &j, GatewayMessage &m) {
    j.at("op").get_to(m.Opcode);
    m.Data = j.at("d");

    if (j.contains("t") && !j.at("t").is_null())
        j.at("t").get_to(m.Type);
}

void from_json(const nlohmann::json &j, HelloMessageData &m) {
    j.at("heartbeat_interval").get_to(m.HeartbeatInterval);
}

void to_json(nlohmann::json &j, const IdentifyProperties &m) {
    j["$os"] = m.OS;
    j["$browser"] = m.Browser;
    j["$device"] = m.Device;
}

void to_json(nlohmann::json &j, const IdentifyMessage &m) {
    j["op"] = GatewayOp::Identify;
    j["d"] = nlohmann::json::object();
    j["d"]["token"] = m.Token;
    j["d"]["properties"] = m.Properties;

    if (m.LargeThreshold)
        j["d"]["large_threshold"] = m.LargeThreshold;
}

void to_json(nlohmann::json &j, const HeartbeatMessage &m) {
    j["op"] = GatewayOp::Heartbeat;
    if (m.Sequence == -1)
        j["d"] = nullptr;
    else
        j["d"] = m.Sequence;
}
