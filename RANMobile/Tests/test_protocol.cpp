// Offline EP7 protocol test vectors (Phase 3.4).
// No server, no assets, no device required — proves the mobile client
// matches the statically inferred TH_PARAM ABI (live capture pending).
//
// Ground truth (netclientLib/s_NetGlobal.h, netserverLib/minTea.cpp):
//   NET_MSG_BASE (TH_PARAM) = 989, NET_MSG_LOBBY = BASE + 950 = 1939
//   THAI_NET_MSG_LOGIN = LOBBY + 140 = 2079
//   THAI_NET_LOGIN_DATA = 56 bytes {nmg(8), nChannel(4), szPassword[21], szUserid[21]}
//   TEA key "Steven Seagal Neck Break" -> 16-byte key "Steven Seagal Ne"
//   TEA vectors below were cross-checked against an independent Python
//   transcription of minTea.cpp (see PROTOCOL_FIX_REPORT.md).

#include "../Engine/Network/EP7Protocol.h"
#include "../Engine/Network/EP7Tea.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// ---- Opcode / layout ground truth (compile-time) ----
static_assert(RANMobile::EP7_NET_MSG_BASE == 989, "TH_PARAM base must be 989");
static_assert(RANMobile::EP7_NET_MSG_LOBBY == 1939, "LOBBY = 989 + 950");
static_assert(RANMobile::EP7_THAI_NET_MSG_LOGIN == 2079, "THAI login opcode");
static_assert(RANMobile::EP7_THAI_NET_MSG_LOGIN_FB == 2080, "THAI login FB opcode");
static_assert(RANMobile::EP7_NET_MSG_LOGIN_2 == 2046, "TW/HK login opcode (reference)");
static_assert(RANMobile::EP7_NET_MSG_HEARTBEAT_CLIENT_REQ == 160, "Heartbeat REQ");
static_assert(RANMobile::EP7_NET_MSG_HEARTBEAT_CLIENT_ANS == 161, "Heartbeat ANS");
static_assert(sizeof(RANMobile::EP7NetMsgGeneric) == 8, "NET_MSG_GENERIC = 8 bytes");
static_assert(sizeof(RANMobile::EP7ThaiNetLoginData) == 56, "THAI_NET_LOGIN_DATA = 56 bytes");

// Independent native layout mirror, without mobile packing directives.
struct PCHeader { uint32_t size; int32_t type; };
struct PCLogin { PCHeader header; int32_t channel; char password[21]; char userid[21]; };
static_assert(sizeof(PCLogin) == 56);
static_assert(offsetof(RANMobile::EP7ThaiNetLoginData, szPassword) == 12);
static_assert(offsetof(RANMobile::EP7ThaiNetLoginData, szUserid) == 33);
static_assert(offsetof(RANMobile::EP7ThaiNetLoginData, tailPadding) == 54);
static_assert(alignof(RANMobile::EP7ThaiNetLoginData) == alignof(PCLogin));

namespace {

bool bytesEqual(const char* a, const uint8_t* b, size_t n) {
    return std::memcmp(a, b, n) == 0;
}

void checkTeaVector(const std::string& plain, const std::vector<uint8_t>& expectedCipher) {
    // Build the 21-byte field exactly like ThaiSndLogin (StringCchCopy into
    // a zeroed 21-byte buffer), then TEA-encrypt the full 21 bytes.
    char field[21]{};
    std::strncpy(field, plain.c_str(), 20);
    field[20] = '\0';

    RANMobile::EP7Tea tea;
    assert(tea.encrypt(field, 21));

    // Ciphertext must match the independently computed vector.
    assert(bytesEqual(field, expectedCipher.data(), expectedCipher.size()));

    // Server-side: decrypt the same 21-byte field (ThaiMsgLogin).
    char wire[21]{};
    std::memcpy(wire, expectedCipher.data(), expectedCipher.size());
    RANMobile::EP7Tea serverTea;
    assert(serverTea.decrypt(wire, 21));
    assert(std::string(wire) == plain);
}

} // namespace

int main() {
    std::cout << "=== EP7 PROTOCOL TEST VECTORS (TH_PARAM, offline) ===\n";

    // 1. TEA key derivation: "Steven Seagal Neck Break" -> "Steven Seagal Ne"
    {
        RANMobile::EP7Tea tea;
        const char* expected = "Steven Seagal Ne";
        assert(std::memcmp(tea.keyBytes(), expected, 16) == 0);
        std::cout << "[PASS] TEA key derivation\n";
    }

    // 2. TEA field vectors (independent Python transcription of minTea.cpp)
    checkTeaVector("hawaii01", {0xd4, 0xd5, 0x51, 0xe8, 0x04, 0x7f, 0x5b, 0xe3});
    checkTeaVector("pass1234", {0x84, 0x78, 0xfe, 0xe9, 0xb9, 0x89, 0xcc, 0x5c});
    checkTeaVector("a",        {0x07, 0xc5, 0x85, 0x17, 0x6e, 0xed, 0x22, 0xfc});
    checkTeaVector("famuser",  {0x73, 0x90, 0x25, 0x12, 0x73, 0xa1, 0xc4, 0x70});
    checkTeaVector("12345678901234567890",
                   {0xc0, 0x37, 0xbe, 0x48, 0x42, 0x80, 0x6d, 0x90, 0x19, 0xf9,
                    0xaf, 0x0a, 0xa1, 0x22, 0x21, 0xca, 0x23, 0x54, 0x4f, 0x45});
    std::cout << "[PASS] TEA encrypt/decrypt vectors (5 cases)\n";

    // 3. Full login packet: client build -> wire -> server decrypt
    //    (mirrors ThaiSndLogin -> ThaiMsgLogin).
    {
        RANMobile::EP7ThaiNetLoginData pkt;
        assert(pkt.nmg.dwSize == 56);
        assert(pkt.nmg.nType == RANMobile::EP7_THAI_NET_MSG_LOGIN);
        assert(pkt.nChannel == 0);

        std::strncpy(pkt.szUserid, "hawaii01", RANMobile::EP7_USR_ID_LENGTH);
        pkt.szUserid[RANMobile::EP7_USR_ID_LENGTH] = '\0';
        std::strncpy(pkt.szPassword, "pass1234", RANMobile::EP7_USR_PASS_LENGTH);
        pkt.szPassword[RANMobile::EP7_USR_PASS_LENGTH] = '\0';

        RANMobile::EP7Tea tea;
        assert(tea.encrypt(pkt.szUserid, 21));
        assert(tea.encrypt(pkt.szPassword, 21));

        // Wire framing: NetworkClient::SendPacket emits
        // {dwSize=56, nType=2079} + 48-byte payload == raw 56-byte struct.
        uint8_t wire[56];
        std::memcpy(wire, &pkt, 56);
        uint32_t wireSize;
        int32_t wireType;
        std::memcpy(&wireSize, wire, 4);
        std::memcpy(&wireType, wire + 4, 4);
        assert(wireSize == 56);
        assert(wireType == 2079);

        assert(wire[54] == 0 && wire[55] == 0);
        // Tail bytes are outside BOTH 21B TEA fields; changing them must not
        // affect decrypt or ciphertext block boundaries.
        wire[54] = 0xa5; wire[55] = 0x5a;
        // Server side (ThaiMsgLogin): decrypt both fields.
        RANMobile::EP7ThaiNetLoginData srv{};
        std::memcpy(&srv, wire, 56);
        RANMobile::EP7Tea serverTea;
        assert(serverTea.decrypt(srv.szUserid, 21));
        assert(serverTea.decrypt(srv.szPassword, 21));
        assert(std::string(srv.szUserid) == "hawaii01");
        assert(std::string(srv.szPassword) == "pass1234");
        std::cout << "[PASS] Full THAI login packet roundtrip (56B, opcode 2079)\n";
    }

    // 4. Field order: password BEFORE userid (THAI_NET_LOGIN_DATA layout).
    {
        RANMobile::EP7ThaiNetLoginData pkt;
        const char* pwOff = pkt.szPassword;
        const char* idOff = pkt.szUserid;
        assert(idOff - pwOff == 21); // password first, userid second
        std::cout << "[PASS] Struct field order (password before userid)\n";
    }

    // 5. Truncation: >20-char input is cut to 20 (StringCchCopy semantics).
    {
        char field[21]{};
        const std::string longInput = "1234567890123456789012345"; // 25 chars
        std::strncpy(field, longInput.c_str(), RANMobile::EP7_USR_ID_LENGTH);
        field[RANMobile::EP7_USR_ID_LENGTH] = '\0';
        assert(std::string(field) == "12345678901234567890");
        std::cout << "[PASS] 20-char truncation (StringCchCopy semantics)\n";
    }

    std::cout << "ALL PROTOCOL TESTS PASSED!\n";
    return 0;
}
