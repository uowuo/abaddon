#ifdef WITH_QRLOGIN

// clang-format off

#include "remoteauthclient.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/fmt/bin_to_hex.h>

#include "abaddon.hpp"
#include "http.hpp"


// clang-format on

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
    if (m_timeout_conn) m_timeout_conn.disconnect();
    m_ws.Stop(1000);
    m_heartbeat_waiter.kill();
    if (m_heartbeat_thread.joinable()) m_heartbeat_thread.join();
}

bool RemoteAuthClient::IsConnected() const noexcept {
    return m_connected;
}

void RemoteAuthClient::OnGatewayMessage(const std::string &str) {
    m_log->trace(str);
    auto j = nlohmann::json::parse(str);
    const auto opcode = j.at("op").get<std::string>();
    if (opcode == "hello") {
        HandleGatewayHello(j);
    } else if (opcode == "nonce_proof") {
        HandleGatewayNonceProof(j);
    } else if (opcode == "pending_remote_init") {
        HandleGatewayPendingRemoteInit(j);
    } else if (opcode == "pending_ticket") {
        HandleGatewayPendingTicket(j);
    } else if (opcode == "pending_login") {
        HandleGatewayPendingLogin(j);
    } else if (opcode == "cancel") {
        HandleGatewayCancel(j);
    }
}

void RemoteAuthClient::HandleGatewayHello(const nlohmann::json &j) {
    const auto timeout_ms = j.at("timeout_ms").get<int>();
    const auto heartbeat_interval = j.at("heartbeat_interval").get<int>();
    m_log->debug("Timeout: {}, Heartbeat: {}", timeout_ms, heartbeat_interval);

    m_heartbeat_msec = heartbeat_interval;
    m_heartbeat_thread = std::thread(&RemoteAuthClient::HeartbeatThread, this);

    m_timeout_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &RemoteAuthClient::OnTimeout), timeout_ms);

    Init();

    m_signal_hello.emit();
}

void RemoteAuthClient::HandleGatewayNonceProof(const nlohmann::json &j) {
    m_log->debug("Received encrypted nonce");

    const auto encrypted_nonce = Glib::Base64::decode(j.at("encrypted_nonce").get<std::string>());
    const auto proof = Decrypt(reinterpret_cast<const unsigned char *>(encrypted_nonce.data()), encrypted_nonce.size());
    auto proof_encoded = Glib::Base64::encode(std::string(proof.begin(), proof.end()));

    std::replace(proof_encoded.begin(), proof_encoded.end(), '/', '_');
    std::replace(proof_encoded.begin(), proof_encoded.end(), '+', '-');
    proof_encoded.erase(std::remove(proof_encoded.begin(), proof_encoded.end(), '='), proof_encoded.end());

    nlohmann::json reply;
    reply["op"] = "nonce_proof";
    reply["nonce"] = proof_encoded;
    m_ws.Send(reply);
}

void RemoteAuthClient::HandleGatewayPendingRemoteInit(const nlohmann::json &j) {
    m_log->debug("Received fingerprint");

    m_signal_fingerprint.emit(j.at("fingerprint").get<std::string>());
}

void RemoteAuthClient::HandleGatewayPendingTicket(const nlohmann::json &j) {
    const auto encrypted_payload = Glib::Base64::decode(j.at("encrypted_user_payload").get<std::string>());
    const auto payload = Decrypt(reinterpret_cast<const unsigned char *>(encrypted_payload.data()), encrypted_payload.size());

    const auto payload_str = std::string(payload.begin(), payload.end());
    m_log->trace("User payload: {}", payload_str);

    const std::vector<Glib::ustring> user_info = Glib::Regex::split_simple(":", payload_str);
    Snowflake user_id;
    std::string discriminator;
    std::string avatar_hash;
    std::string username;
    if (user_info.size() >= 4) {
        user_id = Snowflake(user_info[0]);
        discriminator = user_info[1];
        avatar_hash = user_info[2];
        username = user_info[3];
    }

    m_signal_pending_ticket.emit(user_id, discriminator, avatar_hash, username);
}

void RemoteAuthClient::HandleGatewayPendingLogin(const nlohmann::json &j) {
    Abaddon::Get().GetDiscordClient().RemoteAuthLogin(j.at("ticket").get<std::string>(), sigc::mem_fun(*this, &RemoteAuthClient::OnRemoteAuthLoginResponse));
    m_signal_pending_login.emit();
}

void RemoteAuthClient::HandleGatewayCancel(const nlohmann::json &j) {
    Stop();
    Start();
}

void RemoteAuthClient::OnRemoteAuthLoginResponse(const std::optional<std::string> &encrypted_token, DiscordError err) {
    if (!encrypted_token.has_value()) {
        const auto err_int = static_cast<int>(err);
        m_log->error("Remote auth login failed: {}", err_int);
        if (err == DiscordError::CAPTCHA_REQUIRED) {
            m_signal_error.emit("Discord is requiring a captcha. You must use a web browser to log in.");
        } else {
            m_signal_error.emit("An error occurred. Try again.");
        }
        return;
    }

    const auto encrypted = Glib::Base64::decode(*encrypted_token);
    const auto token = Decrypt(reinterpret_cast<const unsigned char *>(encrypted.data()), encrypted.size());
    m_signal_token.emit(std::string(token.begin(), token.end()));
}

void RemoteAuthClient::Init() {
    GenerateKey();
    const auto key = GetEncodedPublicKey();
    if (key.empty()) {
        m_log->error("Something went wrong");
        // todo disconnect
        return;
    }

    nlohmann::json msg;
    msg["op"] = "init";
    msg["encoded_public_key"] = key;
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
        if (m_connected) {
            m_signal_error.emit("Error. Websocket closed (remote): " + std::to_string(info.code) + " (" + info.reason + ")");
        }
    } else {
        m_log->debug("Websocket closed (local): {} ({})", info.code, info.reason);
        if (m_connected) {
            m_signal_error.emit("Error. Websocket closed (local): " + std::to_string(info.code) + " (" + info.reason + ")");
        }
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

        nlohmann::json hb;
        hb["op"] = "heartbeat";
        m_ws.Send(hb);
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

bool RemoteAuthClient::OnTimeout() {
    m_log->trace("Socket timeout");
    Stop();
    Start();
    return false; // disconnect
}

RemoteAuthClient::type_signal_hello RemoteAuthClient::signal_hello() {
    return m_signal_hello;
}

RemoteAuthClient::type_signal_fingerprint RemoteAuthClient::signal_fingerprint() {
    return m_signal_fingerprint;
}

RemoteAuthClient::type_signal_pending_ticket RemoteAuthClient::signal_pending_ticket() {
    return m_signal_pending_ticket;
}

RemoteAuthClient::type_signal_pending_login RemoteAuthClient::signal_pending_login() {
    return m_signal_pending_login;
}

RemoteAuthClient::type_signal_token RemoteAuthClient::signal_token() {
    return m_signal_token;
}

RemoteAuthClient::type_signal_error RemoteAuthClient::signal_error() {
    return m_signal_error;
}

#endif
