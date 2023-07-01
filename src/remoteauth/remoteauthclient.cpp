#include "remoteauthclient.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/fmt/bin_to_hex.h>

struct RemoteAuthGatewayMessage {
    std::string Opcode;

    friend void from_json(const nlohmann::json &j, RemoteAuthGatewayMessage &m) {
        j.at("op").get_to(m.Opcode);
    }
};

struct RemoteAuthHelloMessage {
    int TimeoutMS;
    int HeartbeatInterval;

    friend void from_json(const nlohmann::json &j, RemoteAuthHelloMessage &m) {
        j.at("timeout_ms").get_to(m.TimeoutMS);
        j.at("heartbeat_interval").get_to(m.HeartbeatInterval);
    }
};

struct RemoteAuthHeartbeatMessage {
    friend void to_json(nlohmann::json &j, const RemoteAuthHeartbeatMessage &m) {
        j["op"] = "heartbeat";
    }
};

struct RemoteAuthInitMessage {
    std::string EncodedPublicKey;

    friend void to_json(nlohmann::json &j, const RemoteAuthInitMessage &m) {
        j["op"] = "init";
        j["encoded_public_key"] = m.EncodedPublicKey;
    }
};

struct RemoteAuthNonceProofMessage {
    std::string EncryptedNonce;
    std::string Nonce;

    friend void from_json(const nlohmann::json &j, RemoteAuthNonceProofMessage &m) {
        j.at("encrypted_nonce").get_to(m.EncryptedNonce);
    }

    friend void to_json(nlohmann::json &j, const RemoteAuthNonceProofMessage &m) {
        j["op"] = "nonce_proof";
        j["nonce"] = m.Nonce;
    }
};

struct RemoteAuthFingerprintMessage {
    std::string Fingerprint;

    friend void from_json(const nlohmann::json &j, RemoteAuthFingerprintMessage &m) {
        j.at("fingerprint").get_to(m.Fingerprint);
    }
};

struct RemoteAuthPendingTicketMessage {
    std::string EncryptedUserPayload;

    friend void from_json(const nlohmann::json &j, RemoteAuthPendingTicketMessage &m) {
        j.at("encrypted_user_payload").get_to(m.EncryptedUserPayload);
    }
};

RemoteAuthClient::RemoteAuthClient()
    : m_ws("remote-auth-ws")
    , m_log(spdlog::get("remote-auth")) {
    m_ws.signal_open().connect(sigc::mem_fun(*this, &RemoteAuthClient::OnWebsocketOpen));
    m_ws.signal_close().connect(sigc::mem_fun(*this, &RemoteAuthClient::OnWebsocketClose));
    m_ws.signal_message().connect(sigc::mem_fun(*this, &RemoteAuthClient::OnWebsocketMessage));

    m_dispatcher.connect(sigc::mem_fun(*this, &RemoteAuthClient::OnDispatch));
}

RemoteAuthClient::~RemoteAuthClient() {
    Stop();
}

void RemoteAuthClient::Start() {
    if (IsConnected()) {
        Stop();
    }

    m_connected = true;
    m_heartbeat_waiter.revive();
    m_ws.StartConnection("wss://remote-auth-gateway.discord.gg/?v=2");
}

void RemoteAuthClient::Stop() {
    if (!IsConnected()) {
        m_log->warn("Requested stop while not connected");
        return;
    }

    m_connected = false;
    m_ws.Stop(1000);
    m_heartbeat_waiter.kill();
    if (m_heartbeat_thread.joinable()) m_heartbeat_thread.join();
}

bool RemoteAuthClient::IsConnected() const noexcept {
    return m_connected;
}

void RemoteAuthClient::OnGatewayMessage(const std::string &str) {
    auto j = nlohmann::json::parse(str);
    RemoteAuthGatewayMessage msg = j;
    if (msg.Opcode == "hello") {
        HandleGatewayHello(j);
    } else if (msg.Opcode == "nonce_proof") {
        HandleGatewayNonceProof(j);
    } else if (msg.Opcode == "pending_remote_init") {
        HandleGatewayPendingRemoteInit(j);
    } else if (msg.Opcode == "pending_ticket") {
        HandleGatewayPendingTicket(j);
    }
}

void RemoteAuthClient::HandleGatewayHello(const nlohmann::json &j) {
    RemoteAuthHelloMessage msg = j;
    m_log->debug("Timeout: {}, Heartbeat: {}", msg.TimeoutMS, msg.HeartbeatInterval);

    m_heartbeat_msec = msg.HeartbeatInterval;
    m_heartbeat_thread = std::thread(&RemoteAuthClient::HeartbeatThread, this);

    Init();
}

void RemoteAuthClient::HandleGatewayNonceProof(const nlohmann::json &j) {
    RemoteAuthNonceProofMessage msg = j;
    m_log->debug("Received encrypted nonce");

    const auto encrypted_nonce = Glib::Base64::decode(msg.EncryptedNonce);
    const auto proof = Decrypt(reinterpret_cast<const unsigned char *>(encrypted_nonce.data()), encrypted_nonce.size());
    auto proof_encoded = Glib::Base64::encode(std::string(proof.begin(), proof.end()));

    std::replace(proof_encoded.begin(), proof_encoded.end(), '/', '_');
    std::replace(proof_encoded.begin(), proof_encoded.end(), '+', '-');
    proof_encoded.erase(std::remove(proof_encoded.begin(), proof_encoded.end(), '='), proof_encoded.end());

    RemoteAuthNonceProofMessage reply;
    reply.Nonce = proof_encoded;
    m_ws.Send(reply);
}

void RemoteAuthClient::HandleGatewayPendingRemoteInit(const nlohmann::json &j) {
    RemoteAuthFingerprintMessage msg = j;
    m_log->debug("Received fingerprint");

    m_signal_fingerprint.emit(msg.Fingerprint);
}

void RemoteAuthClient::HandleGatewayPendingTicket(const nlohmann::json &j) {
    RemoteAuthPendingTicketMessage msg = j;

    const auto encrypted_payload = Glib::Base64::decode(msg.EncryptedUserPayload);
    const auto payload = Decrypt(reinterpret_cast<const unsigned char *>(encrypted_payload.data()), encrypted_payload.size());

    m_log->trace("User payload: {}", std::string(payload.begin(), payload.end()));
}

void RemoteAuthClient::Init() {
    RemoteAuthInitMessage msg;

    GenerateKey();
    msg.EncodedPublicKey = GetEncodedPublicKey();
    if (msg.EncodedPublicKey.empty()) {
        m_log->error("Something went wrong");
        // todo disconnect
        return;
    }

    m_ws.Send(msg);
}

void RemoteAuthClient::GenerateKey() {
    // you javascript people have it so easy
    // check out this documentation https://www.openssl.org/docs/man1.1.1/man3/PEM_write_bio_PUBKEY.html

    m_pkey_ctx.reset(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
    if (!m_pkey_ctx) {
        m_log->error("Failed to create RSA context");
        return;
    }

    if (EVP_PKEY_keygen_init(m_pkey_ctx.get()) <= 0) {
        m_log->error("Failed to initialize RSA context");
        return;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(m_pkey_ctx.get(), 2048) <= 0) {
        m_log->error("Failed to set keygen bits");
        return;
    }

    EVP_PKEY *pkey_tmp = nullptr;
    if (EVP_PKEY_keygen(m_pkey_ctx.get(), &pkey_tmp) <= 0) {
        m_log->error("Failed to generate keypair");
        return;
    }
    m_pkey.reset(pkey_tmp);

    m_dec_ctx.reset(EVP_PKEY_CTX_new(m_pkey.get(), nullptr));
    if (EVP_PKEY_decrypt_init(m_dec_ctx.get()) <= 0) {
        m_log->error("Failed to initialize RSA decrypt context");
        return;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(m_dec_ctx.get(), RSA_PKCS1_OAEP_PADDING) <= 0) {
        m_log->error("EVP_PKEY_CTX_set_rsa_padding failed");
        return;
    }

    if (EVP_PKEY_CTX_set_rsa_oaep_md(m_dec_ctx.get(), EVP_sha256()) <= 0) {
        m_log->error("EVP_PKEY_CTX_set_rsa_oaep_md failed");
        return;
    }

    if (EVP_PKEY_CTX_set_rsa_mgf1_md(m_dec_ctx.get(), EVP_sha256()) <= 0) {
        m_log->error("EVP_PKEY_CTX_set_rsa_mgf1_md");
        return;
    }
}

std::string RemoteAuthClient::GetEncodedPublicKey() const {
    auto bio = BIO_ptr(BIO_new(BIO_s_mem()), BIO_free);
    if (!bio) {
        m_log->error("Failed to create BIO");
        return {};
    }

    if (PEM_write_bio_PUBKEY(bio.get(), m_pkey.get()) <= 0) {
        m_log->error("Failed to write public key to BIO");
        return {};
    }

    // i think this is freed when the bio is too
    BUF_MEM *mem = nullptr;
    if (BIO_get_mem_ptr(bio.get(), &mem) <= 0) {
        m_log->error("Failed to get BIO mem buf");
        return {};
    }

    if (mem->data == nullptr || mem->length == 0) {
        m_log->error("BIO mem buf is null or of zero length");
        return {};
    }

    std::string pem_pubkey(mem->data, mem->length);
    // isolate key
    pem_pubkey.erase(0, pem_pubkey.find("\n") + 1);
    pem_pubkey.erase(pem_pubkey.rfind("\n-"));
    size_t pos;
    while ((pos = pem_pubkey.find("\n")) != std::string::npos) {
        pem_pubkey.erase(pos, 1);
    }
    return pem_pubkey;
}

std::vector<uint8_t> RemoteAuthClient::Decrypt(const unsigned char *in, size_t inlen) const {
    // get length
    size_t outlen;
    if (EVP_PKEY_decrypt(m_dec_ctx.get(), nullptr, &outlen, in, inlen) <= 0) {
        m_log->error("Failed to get length when decrypting");
        return {};
    }

    std::vector<uint8_t> ret(outlen);
    if (EVP_PKEY_decrypt(m_dec_ctx.get(), ret.data(), &outlen, in, inlen) <= 0) {
        m_log->error("Failed to decrypt");
        return {};
    }
    ret.resize(outlen);
    return ret;
}

void RemoteAuthClient::OnWebsocketOpen() {
    m_log->info("Websocket opened");
}

void RemoteAuthClient::OnWebsocketClose(const ix::WebSocketCloseInfo &info) {
    if (info.remote) {
        m_log->debug("Websocket closed (remote): {} ({})", info.code, info.reason);
    } else {
        m_log->debug("Websocket closed (local): {} ({})", info.code, info.reason);
    }
}

void RemoteAuthClient::OnWebsocketMessage(const std::string &data) {
    m_dispatch_mutex.lock();
    m_dispatch_queue.push(data);
    m_dispatcher.emit();
    m_dispatch_mutex.unlock();
}

void RemoteAuthClient::HeartbeatThread() {
    while (true) {
        if (!m_heartbeat_waiter.wait_for(std::chrono::milliseconds(m_heartbeat_msec))) break;

        m_ws.Send(RemoteAuthHeartbeatMessage());
    }
}

void RemoteAuthClient::OnDispatch() {
    m_dispatch_mutex.lock();
    if (m_dispatch_queue.empty()) {
        m_dispatch_mutex.unlock();
        return;
    }
    auto msg = std::move(m_dispatch_queue.front());
    m_dispatch_queue.pop();
    m_dispatch_mutex.unlock();
    OnGatewayMessage(msg);
}

RemoteAuthClient::type_signal_fingerprint RemoteAuthClient::signal_fingerprint() {
    return m_signal_fingerprint;
}
