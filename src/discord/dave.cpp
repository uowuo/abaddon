#ifdef WITH_VOICE
// clang-format off

#include "dave.hpp"
#include "voiceclient.hpp"
#include <dave/logger.h>
#include <spdlog/spdlog.h>
// clang-format on

static bool s_dave_log_sink_set = false;

DaveSession::DaveSession(Snowflake channelId, Snowflake userId,
                         const std::unordered_map<uint32_t, Snowflake> &ssrcUserMap)
    : m_channel_id(channelId)
    , m_user_id(userId)
    , m_ssrc_user_map(ssrcUserMap)
    , m_log(spdlog::get("voice")) {
    if (!s_dave_log_sink_set) {
        s_dave_log_sink_set = true;
        discord::dave::SetLogSink([](discord::dave::LoggingSeverity severity,
                                     const char *file, int line,
                                     const std::string &message) {
            auto log = spdlog::get("voice");
            if (!log) return;
            switch (severity) {
                case discord::dave::LS_ERROR:
                    log->error("[DAVE] {}:{} {}", file, line, message);
                    break;
                case discord::dave::LS_WARNING:
                    log->warn("[DAVE] {}:{} {}", file, line, message);
                    break;
                case discord::dave::LS_INFO:
                    log->debug("[DAVE] {}", message);
                    break;
                case discord::dave::LS_VERBOSE:
                    log->debug("[DAVE] {}", message);
                    break;
                default:
                    break;
            }
        });
    }

    m_mls_session = discord::dave::mls::CreateSession(
        nullptr, "",
        [this](const std::string &reason, const std::string &detail) {
            m_log->warn("MLS failure: {} {}", reason, detail);
        });
}

DaveSession::~DaveSession() = default;

void DaveSession::Init(uint16_t version) {
    m_protocol_version = version;
    m_pending_protocol_version = version;
    Reinit();
}

void DaveSession::Reinit() {
    m_enabled = false;
    m_downgraded = false;

    auto self_id = std::to_string(static_cast<uint64_t>(m_user_id));
    m_connected_users.insert(self_id);

    m_log->info("Initializing DAVE session: channel={} version={} users={}",
                static_cast<uint64_t>(m_channel_id), m_protocol_version, m_connected_users.size());

    m_mls_session->Init(
        m_protocol_version,
        static_cast<uint64_t>(m_channel_id),
        self_id,
        m_transient_key);

    m_decryptors.clear();
    m_pending_transition_ready = false;

    m_encryptor = discord::dave::CreateEncryptor();
    if (m_local_ssrc != 0)
        m_encryptor->AssignSsrcToCodec(m_local_ssrc, discord::dave::Codec::Opus);

    auto keyPackage = m_mls_session->GetMarshalledKeyPackage();
    if (!keyPackage.empty()) {
        m_log->info("Sending MLS key package, size={}", keyPackage.size());
        m_signal_send_binary.emit(static_cast<int>(VoiceGatewayOp::MlsKeyPackage), keyPackage);
    }
}

void DaveSession::OnExternalSenderPackage(const uint8_t *data, size_t size) {
    m_log->info("Received external sender package, size={}", size);
    std::vector<uint8_t> payload(data, data + size);
    m_mls_session->SetExternalSender(payload);
}

void DaveSession::OnProposals(const uint8_t *data, size_t size) {
    m_log->info("Received proposals, size={} connectedUsers={}", size, m_connected_users.size());
    std::vector<uint8_t> payload(data, data + size);
    auto response = m_mls_session->ProcessProposals(std::move(payload), m_connected_users);

    if (response) {
        m_log->info("Sending commit+welcome, size={}", response->size());
        m_signal_send_binary.emit(static_cast<int>(VoiceGatewayOp::MlsCommitWelcome), *response);
    }
}

void DaveSession::OnAnnounceCommitTransition(const uint8_t *data, size_t size) {
    if (size < 2) return;

    int transitionId = (data[0] << 8) | data[1];
    m_pending_transition_id = transitionId;

    m_log->debug("Received announce commit transition: transitionId={} size={}", transitionId, size);

    std::vector<uint8_t> payload(data + 2, data + size);
    auto result = m_mls_session->ProcessCommit(std::move(payload));

    if (auto *roster = std::get_if<discord::dave::RosterMap>(&result)) {
        m_log->info("ProcessCommit succeeded, roster size={}", roster->size());
        m_pending_transition_ready = true;
        m_signal_send_ready.emit(transitionId);
        if (transitionId == 0)
            CompleteTransition();
    } else if (std::holds_alternative<discord::dave::failed_t>(result)) {
        m_log->warn("ProcessCommit failed (hard reject)");
        m_signal_send_invalid.emit(transitionId);
    } else {
        m_log->debug("ProcessCommit ignored (soft reject)");
    }
}

void DaveSession::OnWelcome(const uint8_t *data, size_t size) {
    if (size < 2) return;

    int transitionId = (data[0] << 8) | data[1];
    m_pending_transition_id = transitionId;

    m_log->info("Received welcome: transitionId={} size={} connectedUsers={}",
                transitionId, size, m_connected_users.size());

    std::vector<uint8_t> payload(data + 2, data + size);
    auto roster = m_mls_session->ProcessWelcome(std::move(payload), m_connected_users);

    if (roster) {
        m_log->info("ProcessWelcome succeeded, roster size={}", roster->size());
        m_pending_transition_ready = true;
        m_signal_send_ready.emit(transitionId);
        if (transitionId == 0)
            CompleteTransition();
    } else {
        m_log->warn("ProcessWelcome failed");
        m_signal_send_invalid.emit(transitionId);
    }
}

void DaveSession::OnPrepareTransition(int version, int transitionId) {
    m_log->info("Prepare transition: version={} transitionId={}", version, transitionId);
    m_pending_transition_id = transitionId;
    m_pending_protocol_version = static_cast<uint16_t>(version);
    m_signal_send_ready.emit(transitionId);
}

void DaveSession::OnExecuteTransition(int transitionId) {
    m_log->info("Execute transition: transitionId={}", transitionId);

    if (m_pending_protocol_version != m_protocol_version) {
        m_protocol_version = m_pending_protocol_version;
        if (m_protocol_version == 0) {
            m_log->info("DAVE downgrade to version 0, disabling");
            m_enabled = false;
            m_downgraded = true;
            m_signal_state_changed.emit(false);
            return;
        }
    }

    if (!m_pending_transition_ready) {
        m_log->warn("Execute transition {} but no pending commit/welcome, reinitializing", transitionId);
        Reinit();
        return;
    }

    CompleteTransition();
}

void DaveSession::CompleteTransition() {
    if (!m_pending_transition_ready) {
        m_log->warn("completeTransition but no pending commit/welcome!");
        return;
    }
    m_pending_transition_ready = false;

    auto selfRatchet = m_mls_session->GetKeyRatchet(std::to_string(static_cast<uint64_t>(m_user_id)));
    if (selfRatchet) {
        m_encryptor->SetKeyRatchet(std::move(selfRatchet));
        m_log->info("Refreshed encryptor key ratchet for new epoch");
    } else {
        m_log->warn("Could not get own key ratchet from MLS session");
    }

    for (auto &[ssrc, dec] : m_decryptors) {
        auto it = m_ssrc_user_map.find(ssrc);
        if (it == m_ssrc_user_map.end())
            continue;
        auto ratchet = m_mls_session->GetKeyRatchet(std::to_string(static_cast<uint64_t>(it->second)));
        if (ratchet) {
            dec->TransitionToKeyRatchet(std::move(ratchet));
            m_log->debug("Refreshed decryptor key ratchet for SSRC={}", ssrc);
        }
    }

    if (!m_enabled) {
        m_enabled = true;
        m_downgraded = false;
        m_log->info("DAVE encryption enabled");
    }

    m_signal_state_changed.emit(true);
    m_pending_transition_id = -1;
}

void DaveSession::OnPrepareEpoch(int version, int epoch) {
    m_log->info("Prepare epoch: version={} epoch={}", version, epoch);
    if (epoch == 1) {
        m_protocol_version = static_cast<uint16_t>(version);
        Reinit();
    }
}

discord::dave::IEncryptor *DaveSession::GetEncryptor() {
    return m_encryptor.get();
}

discord::dave::IDecryptor *DaveSession::GetOrCreateDecryptor(uint32_t ssrc, Snowflake uid) {
    auto it = m_decryptors.find(ssrc);
    if (it != m_decryptors.end())
        return it->second.get();

    auto decryptor = discord::dave::CreateDecryptor();

    bool hasRatchet = false;
    if (static_cast<uint64_t>(uid) != 0) {
        auto keyRatchet = m_mls_session->GetKeyRatchet(std::to_string(static_cast<uint64_t>(uid)));
        if (keyRatchet) {
            decryptor->TransitionToKeyRatchet(std::move(keyRatchet));
            hasRatchet = true;
        }
    }

    auto *ptr = decryptor.get();
    m_decryptors.emplace(ssrc, std::move(decryptor));

    m_log->debug("Created decryptor for SSRC={} hasRatchet={}", ssrc, hasRatchet);
    return ptr;
}

void DaveSession::SetLocalSSRC(uint32_t ssrc) {
    m_local_ssrc = ssrc;
    if (m_encryptor)
        m_encryptor->AssignSsrcToCodec(ssrc, discord::dave::Codec::Opus);
}

void DaveSession::AddConnectedUser(const std::string &id) {
    m_connected_users.insert(id);
}

void DaveSession::RemoveConnectedUser(const std::string &id) {
    m_connected_users.erase(id);

    uint64_t uid = std::stoull(id);
    for (auto it = m_ssrc_user_map.begin(); it != m_ssrc_user_map.end(); ++it) {
        if (static_cast<uint64_t>(it->second) == uid) {
            m_decryptors.erase(it->first);
            break;
        }
    }
}

void DaveSession::GetPairwiseFingerprint(const std::string &userId, FingerprintCallback callback) const {
    if (!m_mls_session || !m_enabled) {
        callback({});
        return;
    }
    m_mls_session->GetPairwiseFingerprint(0, userId, std::move(callback));
}

std::vector<uint8_t> DaveSession::GetLastEpochAuthenticator() const {
    if (!m_mls_session || !m_enabled)
        return {};
    return m_mls_session->GetLastEpochAuthenticator();
}

void DaveSession::ApplyKeyRatchetForSSRC(uint32_t ssrc, Snowflake uid) {
    auto it = m_decryptors.find(ssrc);
    if (it == m_decryptors.end())
        return;

    auto keyRatchet = m_mls_session->GetKeyRatchet(std::to_string(static_cast<uint64_t>(uid)));
    if (keyRatchet) {
        it->second->TransitionToKeyRatchet(std::move(keyRatchet));
        m_log->info("Applied key ratchet for SSRC={} user={}", ssrc, static_cast<uint64_t>(uid));
    }
}

DaveSession::type_signal_send_binary DaveSession::signal_send_binary() {
    return m_signal_send_binary;
}

DaveSession::type_signal_send_ready DaveSession::signal_send_ready_for_transition() {
    return m_signal_send_ready;
}

DaveSession::type_signal_send_invalid DaveSession::signal_send_invalid_commit_welcome() {
    return m_signal_send_invalid;
}

DaveSession::type_signal_state_changed DaveSession::signal_dave_state_changed() {
    return m_signal_state_changed;
}

#endif
