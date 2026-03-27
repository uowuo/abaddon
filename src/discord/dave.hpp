#pragma once
#ifdef WITH_VOICE
// clang-format off

#include "snowflake.hpp"
#include <dave/dave_interfaces.h>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <sigc++/sigc++.h>
#include <spdlog/logger.h>
// clang-format on

namespace mlspp {
struct SignaturePrivateKey;
}

class DaveSession {
public:
    DaveSession(Snowflake channelId, Snowflake userId,
                const std::unordered_map<uint32_t, Snowflake> &ssrcUserMap);
    ~DaveSession();

    void Init(uint16_t protocolVersion);

    // binary payloads from server
    void OnExternalSenderPackage(const uint8_t *data, size_t size);
    void OnProposals(const uint8_t *data, size_t size);
    void OnAnnounceCommitTransition(const uint8_t *data, size_t size);
    void OnWelcome(const uint8_t *data, size_t size);

    // json payloads from server
    void OnPrepareTransition(int protocolVersion, int transitionId);
    void OnExecuteTransition(int transitionId);
    void OnPrepareEpoch(int protocolVersion, int epoch);

    discord::dave::IEncryptor *GetEncryptor();
    discord::dave::IDecryptor *GetOrCreateDecryptor(uint32_t ssrc, Snowflake userId);

    bool IsEnabled() const { return m_enabled; }
    bool IsDowngraded() const { return m_downgraded; }

    using FingerprintCallback = std::function<void(const std::vector<uint8_t> &)>;
    void GetPairwiseFingerprint(const std::string &userId, FingerprintCallback callback) const;
    std::vector<uint8_t> GetLastEpochAuthenticator() const;

    void SetLocalSSRC(uint32_t ssrc);
    void AddConnectedUser(const std::string &userId);
    void RemoveConnectedUser(const std::string &userId);
    void ApplyKeyRatchetForSSRC(uint32_t ssrc, Snowflake userId);

    // signal types
    using type_signal_send_binary = sigc::signal<void(int opcode, const std::vector<uint8_t> &data)>;
    using type_signal_send_ready = sigc::signal<void(int transitionId)>;
    using type_signal_send_invalid = sigc::signal<void(int transitionId)>;
    using type_signal_state_changed = sigc::signal<void(bool enabled)>;

    type_signal_send_binary signal_send_binary();
    type_signal_send_ready signal_send_ready_for_transition();
    type_signal_send_invalid signal_send_invalid_commit_welcome();
    type_signal_state_changed signal_dave_state_changed();

private:
    void Reinit();
    void CompleteTransition();

    std::unique_ptr<discord::dave::mls::ISession> m_mls_session;
    std::unique_ptr<discord::dave::IEncryptor> m_encryptor;
    std::unordered_map<uint32_t, std::unique_ptr<discord::dave::IDecryptor>> m_decryptors;

    uint16_t m_protocol_version = 0;
    uint16_t m_pending_protocol_version = 0;
    Snowflake m_channel_id;
    Snowflake m_user_id;
    uint32_t m_local_ssrc = 0;

    std::set<std::string> m_connected_users;
    int m_pending_transition_id = -1;
    bool m_enabled = false;
    bool m_downgraded = false;
    bool m_pending_transition_ready = false;

    const std::unordered_map<uint32_t, Snowflake> &m_ssrc_user_map;

    std::shared_ptr<::mlspp::SignaturePrivateKey> m_transient_key;

    std::shared_ptr<spdlog::logger> m_log;

    type_signal_send_binary m_signal_send_binary;
    type_signal_send_ready m_signal_send_ready;
    type_signal_send_invalid m_signal_send_invalid;
    type_signal_state_changed m_signal_state_changed;
};

#endif
