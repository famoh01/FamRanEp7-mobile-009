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
//   - THAI_NET_LOGIN_DATA (s_NetGlobal.h:2260): 56 bytes, password BEFORE
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

constexpr int32_t EP7_NET_MSG_SND_ENCRYPT_KEY = EP7_NET_MSG_LOBBY + 160; // 2099

constexpr size_t EP7_USR_ID_LENGTH = 20;
constexpr size_t EP7_USR_PASS_LENGTH = 20;

#pragma pack(push, 1)
struct EP7NetMsgGeneric {
    uint32_t dwSize = 0;
    int32_t nType = 0;
};

// THAI_NET_LOGIN_DATA: inferred native PC ABI = 56 bytes.
// Best static evidence; live capture confirmation deferred to Phase 6.
// Field order matters: szPassword BEFORE szUserid.
struct alignas(4) EP7ThaiNetLoginData {
    EP7NetMsgGeneric nmg;
    int32_t nChannel = 0;
    char szPassword[EP7_USR_PASS_LENGTH + 1]{};
    char szUserid[EP7_USR_ID_LENGTH + 1]{};
    // Explicit native-PC tail padding; zeroed to avoid leaking memory on wire.
    uint8_t tailPadding[2]{};

    EP7ThaiNetLoginData() {
        nmg.dwSize = static_cast<uint32_t>(sizeof(EP7ThaiNetLoginData));
        nmg.nType = EP7_THAI_NET_MSG_LOGIN;
        nChannel = 0;
        std::memset(szUserid, 0, sizeof(szUserid));
        std::memset(szPassword, 0, sizeof(szPassword));
    }
};
// --- Phase 3.2 (evidence-backed GCTRL movement, no new opcodes) ---
// Source: netclientLib/s_NetGlobal.h (2007-04-11 active block)
//   NET_MSG_GCTRL = NET_MSG_BASE + 1900  => TH_PARAM 989+1900 = 2889
//   NET_MSG_GCTRL_MOVESTATE = NET_MSG_GCTRL + 140  => 3029
//   NET_MSG_GCTRL_GOTO      = NET_MSG_GCTRL + 142  => 3031
// Source: RanClientLib/G-Logic/GLContrlPcMsg.h
//   struct SNETPC_GOTO { NET_MSG_GENERIC nmg; DWORD dwActState;
//                        D3DXVECTOR3 vCurPos; D3DXVECTOR3 vTarPos; } (pack(1), 36B)
//   struct SNETPC_MOVESTATE { NET_MSG_GENERIC nmg; DWORD dwActState; } (12B)
// Source: RanClientLib/G-Logic/GLCharDefine.h
//   EM_ACT_RUN = 0x00000001
constexpr int32_t EP7_NET_MSG_GCTRL = EP7_NET_MSG_BASE + 1900; // 2889
constexpr int32_t EP7_NET_MSG_GCTRL_MOVESTATE = EP7_NET_MSG_GCTRL + 140; // 3029
constexpr int32_t EP7_NET_MSG_GCTRL_GOTO = EP7_NET_MSG_GCTRL + 142;      // 3031
constexpr uint32_t EP7_EM_ACT_RUN = 0x00000001;

struct EP7Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};
static_assert(sizeof(EP7Vec3) == 12, "D3DXVECTOR3-compatible size");

// SNETPC_GOTO (GLContrlPcMsg.h): client->server movement goto intent.
struct EP7SNetPCGoto {
    EP7NetMsgGeneric nmg;
    uint32_t dwActState = 0;
    EP7Vec3 vCurPos{};
    EP7Vec3 vTarPos{};
    EP7SNetPCGoto() {
        nmg.dwSize = static_cast<uint32_t>(sizeof(EP7SNetPCGoto));
        nmg.nType = EP7_NET_MSG_GCTRL_GOTO;
    }
};

// SNETPC_MOVESTATE (GLContrlPcMsg.h): movement state flag (run/walk).
struct EP7SNetPCMoveState {
    EP7NetMsgGeneric nmg;
    uint32_t dwActState = 0;
    EP7SNetPCMoveState() {
        nmg.dwSize = static_cast<uint32_t>(sizeof(EP7SNetPCMoveState));
        nmg.nType = EP7_NET_MSG_GCTRL_MOVESTATE;
    }
};

#pragma pack(pop)

} // namespace RANMobile

static_assert(sizeof(RANMobile::EP7ThaiNetLoginData) == 56, "Native PC login ABI");
static_assert(sizeof(RANMobile::EP7SNetPCGoto) == 36, "SNETPC_GOTO size");
static_assert(sizeof(RANMobile::EP7SNetPCMoveState) == 12, "SNETPC_MOVESTATE size");
static_assert(RANMobile::EP7_NET_MSG_GCTRL == 2889, "TH GCTRL base");
static_assert(RANMobile::EP7_NET_MSG_GCTRL_MOVESTATE == 3029, "MOVESTATE opcode");
static_assert(RANMobile::EP7_NET_MSG_GCTRL_GOTO == 3031, "GOTO opcode");
