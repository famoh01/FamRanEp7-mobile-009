#include "../Engine/Network/NetworkClient.h"
#include <cassert>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace RANMobile;

static std::vector<uint8_t> Frame(int32_t type, size_t size) {
    std::vector<uint8_t> bytes(size, 0);
    const EP7NetMsgGeneric header{static_cast<uint32_t>(size), type};
    std::memcpy(bytes.data(), &header, sizeof(header));
    return bytes;
}

// Mirror native PC heartbeat layout (no packed pragma in s_NetGlobal.h).
struct NativePCHeader { uint32_t size; int32_t type; };
struct NativeHeartbeatAnswer { NativePCHeader nmg; char key[13]; };
static_assert(sizeof(NativeHeartbeatAnswer) == 24, "PC heartbeat ABI");

int main() {
    const auto login = Frame(EP7_THAI_NET_MSG_LOGIN, 56);
    const auto req = Frame(160, 8);
    // Every possible two-fragment split, including splits inside dwSize/opcode.
    for (size_t split = 1; split < login.size(); ++split) {
        EP7Stream stream;
        std::vector<uint8_t> out;
        for (size_t i = 0; i < split; ++i) {
            assert(stream.Push(login[i], out)); assert(out.empty());
        }
        for (size_t i = split; i < login.size(); ++i) {
            assert(stream.Push(login[i], out));
            if (i + 1 < login.size()) assert(out.empty());
        }
        assert(out == login);
    }
    EP7Stream stream;
    std::vector<uint8_t> wire = login, out;
    wire.insert(wire.end(), req.begin(), req.end());
    wire.insert(wire.end(), login.begin(), login.begin() + 7);
    size_t count = 0;
    for (const auto b : wire) {
        assert(stream.Push(b, out));
        if (!out.empty()) { assert(out == (count == 0 ? login : req)); ++count; }
    }
    assert(count == 2);
    for (size_t i = 7; i < login.size(); ++i) assert(stream.Push(login[i], out));
    assert(out == login);
    for (const uint32_t invalid : {0u, 1u, 7u, 1025u, 0xffffffffu}) {
        stream.Reset();
        auto bad = req; std::memcpy(bad.data(), &invalid, sizeof(invalid));
        for (size_t i = 0; i < 7; ++i) assert(stream.Push(bad[i], out));
        assert(!stream.Push(bad[7], out));
    }
    stream.Reset();
    const auto maxFrame = Frame(2000, 1024);
    for (const auto b : maxFrame) assert(stream.Push(b, out));
    assert(out == maxFrame);
    for (size_t i = 0; i < 7; ++i) assert(stream.Push(login[i], out));
    stream.Reset(); // reconnect discards incomplete old frame
    for (const auto b : req) assert(stream.Push(b, out));
    assert(out == req);
    std::cout << "PASS: every split, bytewise, coalesced, trailing partial, invalid sizes, reset, 1024B\n";

    EP7Heartbeat heartbeat;
    std::vector<uint8_t> answer;
    assert(heartbeat.Handle(160, req.data(), req.size(), answer));
    assert(answer == Frame(161, 24)); // Login/Field golden wire (zero key + padding)
    auto key = Frame(EP7_NET_MSG_SND_ENCRYPT_KEY, 24);
    std::memcpy(key.data() + 8, "hawaii01", 8);
    assert(heartbeat.Handle(EP7_NET_MSG_SND_ENCRYPT_KEY, key.data(), key.size(), answer));
    assert(answer.empty());
    assert(heartbeat.Handle(160, req.data(), req.size(), answer));
    const uint8_t golden[] = {0xd4,0xd5,0x51,0xe8,0x04,0x7f,0x5b,0xe3};
    assert(answer.size() == 24);
    assert(std::memcmp(answer.data() + 8, golden, sizeof(golden)) == 0);
    char rolling[13]{}; std::memcpy(rolling, golden, sizeof(golden));
    EP7Tea tea; assert(tea.encrypt(rolling, 13));
    assert(heartbeat.Handle(160, req.data(), req.size(), answer));
    assert(std::memcmp(answer.data() + 8, rolling, 13) == 0);
    assert(!heartbeat.Handle(160, req.data(), 7, answer));
    assert(!heartbeat.Handle(EP7_NET_MSG_SND_ENCRYPT_KEY, key.data(), 23, answer));
    heartbeat.Reset();
    assert(heartbeat.Handle(160, req.data(), req.size(), answer));
    assert(answer == Frame(161, 24));
    std::cout << "PASS: heartbeat golden wire, agent key/rolling TEA, malformed sizes, reset\n";

    // Offline loopback transport fixture, no EP7 server, assets, sleeps or clock dependency.
    const int listener = ::socket(AF_INET, SOCK_STREAM, 0); assert(listener >= 0);
    sockaddr_in addr{}; addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    assert(::bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    socklen_t len = sizeof(addr);
    assert(::getsockname(listener, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
    assert(::listen(listener, 1) == 0);
    NetworkClient client;
    assert(client.Connect("127.0.0.1", ntohs(addr.sin_port)));
    const int peer = ::accept(listener, nullptr, nullptr); assert(peer >= 0);
    size_t callbacks = 0;
    client.SetOnPacketCallback([&](int32_t type, const uint8_t* data, uint32_t size) {
        assert(type == 160 && size == 8 && std::memcmp(data, req.data(), 8) == 0);
        ++callbacks;
    });
    client.Poll(1000000.0f); // no unsolicited heartbeat, regardless of elapsed time
    uint8_t received[64]{};
    assert(::recv(peer, received, sizeof(received), MSG_DONTWAIT) == -1);
    assert(::send(peer, req.data(), 3, 0) == 3);
    client.Poll(0); assert(callbacks == 0);
    assert(::send(peer, req.data() + 3, 5, 0) == 5);
    // Poll is nonblocking: bounded retries allow the loopback stack to deliver bytes.
    for (int i = 0; i < 10000 && callbacks == 0; ++i) client.Poll(0);
    assert(callbacks == 1);
    assert(::recv(peer, received, 24, MSG_WAITALL) == 24);
    const auto expected = Frame(161, 24);
    assert(std::memcmp(received, expected.data(), 24) == 0);
    ::close(peer);
    for (int i = 0; i < 10000 && client.IsConnected(); ++i) client.Poll(0);
    assert(!client.IsConnected()); ::close(listener);
    std::cout << "PASS: actual Poll fragmentation, callback size, immediate reply, no timer, EOF\n";
}
