// Phase 3.4 Protocol Hardening — offline, evidence-based only.
// No new opcodes/structs invented. Tests extend coverage for behaviors already
// present in imported PR #2 baseline code and cited source evidence.
//
// Evidence references:
// - EP7Protocol.h: TH_PARAM base/lobby/opcodes/sizes (s_NetGlobal.h)
// - EP7Tea.cpp/h: minTea port (netserverLib/minTea.cpp,h)
// - EP7Stream.h: raw frame dwSize validation (NET_DATA_BUFSIZE=1024)
// - EP7Heartbeat (EP7Stream.h): SND_ENCRYPT_KEY 24B, REQ 8B, ANS 24B, rolling TEA
// - NetworkClient.cpp: SendPacket size guard, disconnected guard, partial send queue

#include "../Engine/Network/EP7Protocol.h"
#include "../Engine/Network/EP7Tea.h"
#include "../Engine/Network/EP7Stream.h"
#include "../Engine/Network/NetworkClient.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include <array>

using namespace RANMobile;

static std::vector<uint8_t> MakeFrame(int32_t type, uint32_t size) {
    std::vector<uint8_t> b(size, 0);
    EP7NetMsgGeneric h{size, type};
    std::memcpy(b.data(), &h, sizeof(h));
    return b;
}

static void test_tea_edge_cases() {
    // EP7Tea::setKey(nullptr) returns false (EP7Tea.h/cpp contract)
    {
        EP7Tea tea;
        assert(tea.setKey(nullptr) == false);
    }
    // empty key is rejected by EP7Tea::setKey (returns false, see EP7Tea.cpp)
    {
        EP7Tea tea;
        assert(tea.setKey("") == false);
        // keyBytes remain default from constructor
        const char* expected = "Steven Seagal Ne";
        assert(std::memcmp(tea.keyBytes(), expected, 16) == 0);
    }
    // encrypt/decrypt null or invalid length should return false
    {
        EP7Tea tea;
        assert(tea.encrypt(nullptr, 21) == false);
        assert(tea.decrypt(nullptr, 21) == false);
        char buf[21] = {};
        assert(tea.encrypt(buf, 0) == false);
        assert(tea.decrypt(buf, 0) == false);
        assert(tea.encrypt(buf, -1) == false);
        assert(tea.decrypt(buf, -1) == false);
    }
    // 21-byte all-zero field: encrypt then decrypt roundtrip, result stays NUL string
    {
        char field[21] = {};
        EP7Tea tea;
        bool enc = tea.encrypt(field, 21);
        // minTea trims trailing NULs; all-zero becomes padded block; we only assert
        // no crash and decrypt returns a NUL-terminated empty string.
        assert(enc == true);
        EP7Tea tea2;
        assert(tea2.decrypt(field, 21) == true);
        assert(field[0] == '\0');
    }
    // custom key roundtrip (evidence: setKey repeats/truncates to 16 bytes)
    {
        EP7Tea tea("ABCDEFGHIJKLMNOPQRST"); // >16 chars, should truncate to 16
        const char* expected16 = "ABCDEFGHIJKLMNOP";
        assert(std::memcmp(tea.keyBytes(), expected16, 16) == 0);
        char field[21] = {};
        std::strncpy(field, "roundtrip", 20);
        char original[21] = {};
        std::memcpy(original, field, 21);
        assert(tea.encrypt(field, 21));
        assert(std::memcmp(field, original, 21) != 0);
        EP7Tea dec("ABCDEFGHIJKLMNOPQRST");
        assert(dec.decrypt(field, 21));
        assert(std::string(field) == "roundtrip");
    }
    std::cout << "[PASS] TEA edge cases (null/empty/invalid len/all-zero/custom key)\n";
}

static void test_stream_edge_cases() {
    // exact 8-byte header-only frame (minimum valid dwSize)
    {
        EP7Stream s;
        std::vector<uint8_t> out;
        auto f = MakeFrame(999, 8);
        for (auto b : f) assert(s.Push(b, out));
        assert(out == f);
    }
    // 1024-byte frame is valid max (NET_DATA_BUFSIZE=1024)
    {
        EP7Stream s;
        std::vector<uint8_t> out;
        auto f = MakeFrame(2001, 1024);
        for (size_t i = 0; i < f.size(); ++i) {
            assert(s.Push(f[i], out));
            if (i + 1 < f.size()) assert(out.empty());
        }
        assert(out == f);
    }
    // invalid dwSize values rejected at header completion
    for (uint32_t bad : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 1025u, 4096u, 0xFFFFFFFFu}) {
        EP7Stream s;
        std::vector<uint8_t> out;
        auto f = MakeFrame(160, 8);
        std::memcpy(f.data(), &bad, 4);
        for (size_t i = 0; i < 7; ++i) assert(s.Push(f[i], out));
        assert(!s.Push(f[7], out));
    }
    // after invalid frame, stream can recover with a fresh valid frame
    {
        EP7Stream s;
        std::vector<uint8_t> out;
        auto bad = MakeFrame(160, 8);
        uint32_t bsz = 0;
        std::memcpy(bad.data(), &bsz, 4);
        for (size_t i = 0; i < 7; ++i) assert(s.Push(bad[i], out));
        assert(!s.Push(bad[7], out)); // reset inside
        auto ok = MakeFrame(160, 8);
        for (auto b : ok) assert(s.Push(b, out));
        assert(out == ok);
    }
    // coalesced 3 valid frames delivered in order
    {
        EP7Stream s;
        std::vector<uint8_t> out;
        auto a = MakeFrame(160, 8);
        auto b = MakeFrame(EP7_THAI_NET_MSG_LOGIN, 56);
        auto c = MakeFrame(161, 24);
        std::vector<uint8_t> wire;
        wire.insert(wire.end(), a.begin(), a.end());
        wire.insert(wire.end(), b.begin(), b.end());
        wire.insert(wire.end(), c.begin(), c.end());
        std::vector<std::vector<uint8_t>> got;
        for (auto by : wire) {
            assert(s.Push(by, out));
            if (!out.empty()) got.push_back(out);
        }
        assert(got.size() == 3);
        assert(got[0] == a);
        assert(got[1] == b);
        assert(got[2] == c);
    }
    std::cout << "[PASS] EP7Stream edge cases (min/max/invalid/recover/coalesced)\n";
}

static void test_heartbeat_edge_cases() {
    // multiple rolling key rounds (evidence: EP7Heartbeat encrypts key each REQ)
    {
        EP7Heartbeat hb;
        auto req = MakeFrame(EP7_NET_MSG_HEARTBEAT_CLIENT_REQ, 8);
        auto keyMsg = MakeFrame(EP7_NET_MSG_SND_ENCRYPT_KEY, 24);
        const char* k = "hawaii01";
        std::memcpy(keyMsg.data() + 8, k, std::strlen(k));
        std::vector<uint8_t> ans;
        assert(hb.Handle(EP7_NET_MSG_SND_ENCRYPT_KEY, keyMsg.data(), keyMsg.size(), ans));
        assert(ans.empty());
        // Round 1: expected golden from test_stream/protocol evidence
        const uint8_t golden[] = {0xd4,0xd5,0x51,0xe8,0x04,0x7f,0x5b,0xe3};
        assert(hb.Handle(160, req.data(), req.size(), ans));
        assert(ans.size() == 24);
        assert(std::memcmp(ans.data() + 8, golden, 8) == 0);
        // Rounds 2-5: each must equal TEA-encrypt of previous key buffer (13 bytes)
        char rolling[13] = {};
        std::memcpy(rolling, golden, 8);
        for (int round = 2; round <= 5; ++round) {
            EP7Tea tea;
            assert(tea.encrypt(rolling, 13));
            assert(hb.Handle(160, req.data(), req.size(), ans));
            assert(ans.size() == 24);
            assert(std::memcmp(ans.data() + 8, rolling, 13) == 0);
        }
        // Reset returns to login/field behavior (zero key)
        hb.Reset();
        assert(hb.Handle(160, req.data(), req.size(), ans));
        assert(ans == MakeFrame(EP7_NET_MSG_HEARTBEAT_CLIENT_ANS, 24));
    }
    // malformed sizes rejected
    {
        EP7Heartbeat hb;
        auto req = MakeFrame(160, 8);
        auto key = MakeFrame(EP7_NET_MSG_SND_ENCRYPT_KEY, 24);
        std::vector<uint8_t> ans;
        assert(!hb.Handle(160, req.data(), 7, ans));
        assert(!hb.Handle(160, req.data(), 9, ans));
        assert(!hb.Handle(EP7_NET_MSG_SND_ENCRYPT_KEY, key.data(), 23, ans));
        assert(!hb.Handle(EP7_NET_MSG_SND_ENCRYPT_KEY, key.data(), 25, ans));
        // wrong type but valid size should not produce answer and not crash
        assert(hb.Handle(99999, req.data(), 8, ans));
        assert(ans.empty());
    }
    std::cout << "[PASS] Heartbeat edge cases (rolling rounds/malformed sizes/reset)\n";
}

static void test_networkclient_guards() {
    NetworkClient c;
    // disconnected send fails
    assert(c.SendPacket(EP7_THAI_NET_MSG_LOGIN, "x", 1) == false);
    // oversize payload rejected (payload > 1024 - 8)
    std::vector<uint8_t> big(1024 - 8 + 1, 0);
    assert(c.SendPacket(EP7_THAI_NET_MSG_LOGIN, big.data(), static_cast<uint32_t>(big.size())) == false);
    // null data with nonzero size rejected
    assert(c.SendPacket(EP7_THAI_NET_MSG_LOGIN, nullptr, 10) == false);
    // zero-size with null is allowed by guard? code: if (size && !data) return false; so size==0,null is ok for guard
    // but disconnected still returns false
    assert(c.SendPacket(EP7_THAI_NET_MSG_LOGIN, nullptr, 0) == false);
    std::cout << "[PASS] NetworkClient guards (disconnected/oversize/null payload)\n";
}

int main() {
    std::cout << "=== Phase 3.4 Protocol Hardening (offline) ===\n";
    test_tea_edge_cases();
    test_stream_edge_cases();
    test_heartbeat_edge_cases();
    test_networkclient_guards();
    std::cout << "ALL PROTOCOL HARDENING TESTS PASSED!\n";
    return 0;
}
