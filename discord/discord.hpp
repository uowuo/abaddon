#pragma once
#include "websocket.hpp"
#include <nlohmann/json.hpp>
#include <thread>
#include <unordered_map>

enum class GatewayOp : int {
    Event = 0,
    Heartbeat = 1,
    Identify = 2,
    Hello = 10,
    HeartbeatAck = 11,
};

enum class GatewayEvent : int {
    READY,
};

struct GatewayMessage {
    GatewayOp Opcode;
    nlohmann::json Data;
    std::string Type;

    friend void from_json(const nlohmann::json &j, GatewayMessage &m);
};

struct HelloMessageData {
    int HeartbeatInterval;

    friend void from_json(const nlohmann::json &j, HelloMessageData &m);
};

struct ReadyEventData {
    std::string AnalyticsToken; // opt

};

struct IdentifyProperties {
    std::string OS;
    std::string Browser;
    std::string Device;

    friend void to_json(nlohmann::json &j, const IdentifyProperties &m);
};

struct IdentifyMessage : GatewayMessage {
    std::string Token;
    IdentifyProperties Properties;
    bool DoesSupportCompression = false;
    int LargeThreshold = 0;

    friend void to_json(nlohmann::json &j, const IdentifyMessage &m);
};

struct HeartbeatMessage : GatewayMessage {
    int Sequence;

    friend void to_json(nlohmann::json &j, const HeartbeatMessage &m);
};

// https://stackoverflow.com/questions/29775153/stopping-long-sleep-threads/29775639#29775639
class HeartbeatWaiter {
public:
    template<class R, class P>
    bool wait_for(std::chrono::duration<R, P> const &time) const {
        std::unique_lock<std::mutex> lock(m);
        return !cv.wait_for(lock, time, [&] { return terminate; });
    }
    void kill() {
        std::unique_lock<std::mutex> lock(m);
        terminate = true;
        cv.notify_all();
    }

private:
    mutable std::condition_variable cv;
    mutable std::mutex m;
    bool terminate = false;
};

class Abaddon;
class DiscordClient {
public:
    static const constexpr char *DiscordGateway = "wss://gateway.discord.gg/?v=6&encoding=json";
    static const constexpr char *DiscordAPI = "https://discord.com/api";
    static const constexpr char *GatewayIdentity = "Discord";

public:
    DiscordClient();
    void SetAbaddon(Abaddon *ptr);
    void Start();
    void Stop();
    bool IsStarted() const;

private:
    void HandleGatewayMessage(nlohmann::json msg);
    void HandleGatewayReady(const GatewayMessage &msg);
    void HeartbeatThread();
    void SendIdentify();

    Abaddon *m_abaddon = nullptr;

    Websocket m_websocket;
    bool m_client_connected = false;
    std::unordered_map<std::string, GatewayEvent> m_event_map;
    void LoadEventMap();

    std::thread m_heartbeat_thread;
    int m_last_sequence = -1;
    int m_heartbeat_msec = 0;
    HeartbeatWaiter m_heartbeat_waiter;
    bool m_heartbeat_acked = true;
};
