#pragma once

#include <cstdint>
#include <cstring>

namespace RANMobile {

// ============================================================================
// EP7 protocol ground truth — verified from PC source (TH_PARAM build).
//
// Source: netclientLib/s_NetGlobal.h
//   - NET_MSG_BASE = 989 for TH_PARAM (s_NetGlobal.h:629-630)
//   - NET_MSG_LOBBY = NET_MSG_BASE + 950 (s_NetGlobal.h:656, 2007-04-11 block
//     — this is the ACTIVE define; the +1013 block at :639 is inside a
//     /* ... */ comment and must NOT be used)
//   - THAI_NET_MSG_LOGIN = NET_MSG_LOBBY + 140 (s_NetGlobal.h:757)
//     => 989 + 950 + 140 = 2079
//   - NET_MSG_LOGIN_2 = NET_MSG_LOBBY + 107 (s_NetGlobal.h:724) => 2046
//     (Taiwan/Hong Kong path — kept for reference only; the Thai build
//     uses THAI_NET_MSG_LOGIN, see LoginPage.cpp:228 -> ThaiSndLogin)
//   - Heartbeat opcodes are ABSOLUTE enum values, not base-relative
//     (s_NetGlobal.h:682-683): REQ = 160, ANS = 161
//   - THAI_NET_LOGIN_DATA (s_NetGlobal.h:2260): 54 bytes, password BEFORE
//     userid, NO szRandomPassword / szEnCrypt fields
//   - NET_MSG_GENERIC (s_NetGlobal.h:2050): 8 bytes { DWORD dwSize; EMNET_MSG nType }
//   - USR_ID_LENGTH = 20, USR_PASS_LENGTH = 20 (s_NetGlobal.h:161-162)
//   - TEA key "Steven Seagal Neck Break" (netserverLib/minTea.cpp:20)
// ============================================================================

// Region base for the Thai build (TH_PARAM). Do NOT use 0 — there is no
// region with base 0 in EP7.
constexpr int32_t EP7_NET_MSG_BASE = 989;
constexpr int32_t EP7_NET_MSG_LOBBY = EP7_NET_MSG_BASE + 950; // 1939

// Absolute (region-independent) opcodes.
constexpr int32_t EP7_NET_MSG_HEARTBEAT_CLIENT_REQ = 160;
constexpr int32_t EP7_NET_MSG_HEARTBEAT_CLIENT_ANS = 161;

// Taiwan/Hong Kong login path (reference only — NOT used by the Thai build).
constexpr int32_t EP7_NET_MSG_LOGIN_2 = EP7_NET_MSG_LOBBY + 107; // 2046
constexpr int32_t EP7_NET_MSG_LOGIN_FB = EP7_NET_MSG_LOBBY + 108; // 2047

// Thailand login path — the one the TH_PARAM build actually uses
// (Basic.cpp:71 EMSERVICE_THAILAND -> LoginPage.cpp:228 ThaiSndLogin).
constexpr int32_t EP7_THAI_NET_MSG_LOGIN = EP7_NET_MSG_LOBBY + 140; // 2079
constexpr int32_t EP7_THAI_NET_MSG_LOGIN_FB = EP7_NET_MSG_LOBBY + 141; // 2080

constexpr size_t EP7_USR_ID_LENGTH = 20;
constexpr size_t EP7_USR_PASS_LENGTH = 20;

#pragma pack(push, 1)
struct EP7NetMsgGeneric {
    uint32_t dwSize = 0;
    int32_t nType = 0;
};

// THAI_NET_LOGIN_DATA (s_NetGlobal.h:2260) — 54 bytes on the wire.
// Field order matters: szPassword BEFORE szUserid.
struct EP7ThaiNetLoginData {
    EP7NetMsgGeneric nmg;
    int32_t nChannel = 0;
    char szPassword[EP7_USR_PASS_LENGTH + 1]{};
    char szUserid[EP7_USR_ID_LENGTH + 1]{};

    EP7ThaiNetLoginData() {
        nmg.dwSize = static_cast<uint32_t>(sizeof(EP7ThaiNetLoginData));
        nmg.nType = EP7_THAI_NET_MSG_LOGIN;
        nChannel = 0;
        std::memset(szUserid, 0, sizeof(szUserid));
        std::memset(szPassword, 0, sizeof(szPassword));
    }
};
#pragma pack(pop)

} // namespace RANMobile
