#include "NetworkClient.h"
#include "EP7Protocol.h"
#include "../Core/Logger.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>

namespace RANMobile {

NetworkClient::~NetworkClient() {
    Disconnect();
}

bool NetworkClient::Connect(const std::string& host, uint16_t port) {
    Disconnect();
    m_host = host;
    m_port = port;
    m_state = ConnectionState::Connecting;

    m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        Logger::Error("NetworkClient", "Failed to create socket");
        m_state = ConnectionState::Disconnected;
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        Logger::Error("NetworkClient", "Invalid server address: " + host);
        ::close(m_socket);
        m_socket = -1;
        m_state = ConnectionState::Disconnected;
        return false;
    }

    if (::connect(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        Logger::Warn("NetworkClient", "Could not connect to EP7 server " + host + ":" + std::to_string(port));
        ::close(m_socket);
        m_socket = -1;
        m_state = ConnectionState::Disconnected;
        return false;
    }

    int flags = ::fcntl(m_socket, F_GETFL, 0);
    if (flags < 0 || ::fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        Disconnect();
        return false;
    }

    m_state = ConnectionState::Connected;
    Logger::Info("NetworkClient", "Connected to EP7 server " + host + ":" + std::to_string(port));
    return true;
}

void NetworkClient::Disconnect() {
    ++m_generation;
    if (m_socket >= 0) {
        ::close(m_socket);
        m_socket = -1;
    }
    m_state = ConnectionState::Disconnected;
    m_stream.Reset();
    m_heartbeat.Reset();
    m_pendingSend.clear();
}

void NetworkClient::Poll(float deltaTime) {
    (void)deltaTime; // EP7 heartbeat is request-driven, not a client timer.
    if (!IsConnected() || m_socket < 0 || !FlushSend()) return;
    const uint64_t generationAtStart = m_generation;
    uint8_t buffer[4096];
    for (;;) {
        const ssize_t received = ::recv(m_socket, buffer, sizeof(buffer), 0);
        if (received < 0) {
            if (errno == EINTR) continue;
            if (errno != EAGAIN && errno != EWOULDBLOCK) Disconnect();
            return;
        }
        if (received == 0) { Disconnect(); return; }
        for (ssize_t i = 0; i < received; ++i) {
            std::vector<uint8_t> frame;
            if (!m_stream.Push(buffer[i], frame)) {
                Logger::Warn("NetworkClient", "Invalid EP7 frame size");
                Disconnect();
                return;
            }
            if (frame.empty()) continue;
            EP7NetMsgGeneric header{};
            std::memcpy(&header, frame.data(), sizeof(header));
            std::vector<uint8_t> answer;
            if (!m_heartbeat.Handle(header.nType, frame.data(), frame.size(), answer)) {
                Disconnect();
                return;
            }
            if (!answer.empty() && !SendPacket(EP7_NET_MSG_HEARTBEAT_CLIENT_ANS,
                    answer.data() + sizeof(header),
                    static_cast<uint32_t>(answer.size() - sizeof(header)))) return;
            if (m_onPacket) m_onPacket(header.nType, frame.data(), header.dwSize);
            // Callback may disconnect/reconnect; never process bytes from the old socket.
            if (!IsConnected() || m_generation != generationAtStart) return;
        }
    }
}

bool NetworkClient::FlushSend() {
    while (!m_pendingSend.empty()) {
        const ssize_t sent = ::send(m_socket, m_pendingSend.data(), m_pendingSend.size(), MSG_NOSIGNAL);
        if (sent < 0 && errno == EINTR) continue;
        if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return true;
        if (sent <= 0) { Disconnect(); return false; }
        m_pendingSend.erase(m_pendingSend.begin(), m_pendingSend.begin() + sent);
    }
    return true;
}

bool NetworkClient::SendPacket(int32_t msgType, const void* data, uint32_t size) {
    if (m_state != ConnectionState::Connected || m_socket < 0) {
        return false;
    }
    if (size > 1024 - sizeof(EP7NetMsgGeneric) || (size && !data)) return false;
    // Queue unsent tails: TCP send() may accept only part of a frame.
    const EP7NetMsgGeneric header{static_cast<uint32_t>(sizeof(EP7NetMsgGeneric)) + size, msgType};
    const auto* bytes = reinterpret_cast<const uint8_t*>(&header);
    m_pendingSend.insert(m_pendingSend.end(), bytes, bytes + sizeof(header));
    if (size) {
        const auto* payload = static_cast<const uint8_t*>(data);
        m_pendingSend.insert(m_pendingSend.end(), payload, payload + size);
    }
    return FlushSend(); // true = accepted/queued, not necessarily delivered yet.
}

} // namespace RANMobile
