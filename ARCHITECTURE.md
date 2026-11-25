# ARCHITECTURE.md — RAN Online EP7 · PC → Mobile (Android) Full Port

> **Phase 1 deliverable** (โครงสร้าง/สถาปัตยกรรมตามหลักฐานจากซอร์สจริง)
> Repo งาน: `famoh01/FamRanEp7-mobile-009` · branch `arena/f23bb3f3-famranep7-mobile-009`
> Upstream (ซอร์สจริง + งานเดิม): `famoh01/ran-online-ep7` (`main` @ `47f3c6d`)
> วันที่ตรวจ: 2026-11-25 · ทุกตัวเลขในเอกสารนี้วัดจากไฟล์จริง ไม่ได้ประมาณ

---

## 0. บทสรุปผู้บริหาร (อ่าน 30 วินาที)

| ประเด็น | ข้อสรุป |
|---|---|
| สถาปัตยกรรมเดิม | **Monolithic Win32 / MFC / DirectX 9 (D3D9)** — 40 MSBuild projects ใน 2 solutions (`Smtm.sln`, `Smtm2022.sln`) |
| Client / Server แยกกันตรงไหน | แยกเป็น **คนละ project ใน solution เดียวกัน** ไม่ใช่คนละ codebase — แชร์ header protocol ตัวเดียวกันคือ `netclientLib/s_NetGlobal.h` |
| งาน mobile เดิม | **มีอยู่แล้ว** `RANMobile/` = 4,434 LOC C++17 + Android NDK/GLES3, CI เขียว (APK build ได้จริง) — **ไม่ต้องเริ่มใหม่** |
| Engine ฝั่ง mobile | เขียนใหม่แบบ portable (GLES3) ไม่ได้พอร์ต `enginelib/` (386 cpp) ตรงๆ และ**ยังไม่ได้ผูก geometry/mesh/map renderer เข้าเกมจริง** |
| ⚠️ Defect สำคัญที่พบ | `RANMobile/Engine/Network/EP7Protocol.h` hardcode `NET_MSG_BASE = 0` → ได้ Login opcode **1120** แต่ build ไทย (`TH_PARAM`) ต้องเป็น **2142** + ต้อง TEA-encrypt ⇒ **ต่อ server จริงไม่ได้** (ดู §7) |
| ⚠️ ข้อจำกัดสำคัญ | `RanLogic/`, `RanLogicClient/`, `RanLogicServer/` (game logic แชร์) **ไม่มีใน repo นี้** ทำให้ port ฝั่ง server ทำได้แค่ระดับ protocol layer |
| Asset ภาพ/โลโก้ต้นฉบับ | **ไม่อยู่ใน Git** (`.rcc/.dds/.ntk` = 0 ไฟล์) อยู่ใน Release 6.5 GB + Google Drive ที่ sandbox โหลดไม่ได้ ⇒ Phase 5 ต้องรอ asset (ดู `BLOCKERS.md`) |
| ดูรายละเอียด Phase 2 | `PROGRESS_REVIEW.md` |

---

## 1. วิธีการสำรวจ (reproducible)

```bash
# 1) ดึงซอร์สจริง (shallow) — 9,982 ไฟล์ / 223 MB
git clone --depth 1 https://github.com/famoh01/ran-online-ep7.git /tmp/ransrc

# 2) ตรวจ protocol ground truth
grep -n "NET_MSG_BASE"            /tmp/ransrc/netclientLib/s_NetGlobal.h
grep -n "THAI_NET_MSG_LOGIN"      /tmp/ransrc/netclientLib/s_NetGlobal.h
grep -n "THAI_NET_LOGIN_DATA" -A22 /tmp/ransrc/netclientLib/s_NetGlobal.h
sed -n '282,315p'                 /tmp/ransrc/netclientLib/s_NetClientMsgLogin.cpp   # ThaiSndLogin
grep -n "EMSERVICE_THAILAND" -B2   /tmp/ransrc/Basic/Basic.cpp

# 3) ตรวจว่า build ไหนเปิด TH_PARAM
grep -o "<PreprocessorDefinitions>[^<]*" /tmp/ransrc/netserverLib/netserverLib.vcxproj

# 4) ตรวจงาน mobile เดิม
ls -R /tmp/ransrc/RANMobile
g++ -std=c++17 -Wall -Wextra -Werror <sources...> -lpthread -o ranmobile_tests   # compile ผ่าน
```

ผลการ compile งานเดิม: **ผ่าน 0 error 0 warning** (`-Wall -Wextra -Werror`) ด้วย g++ 13
ผลการรัน test ในเครื่องนี้: abort ที่ `test_main.cpp:186` **เพราะไม่มี asset** (`thaipack_original`) ไม่ใช่ bug ของโค้ด — CI (ซึ่งโหลด asset ได้) เขียว ดู §5

### แหล่งข้อมูลที่เข้าถึงได้ / ไม่ได้ (สรุป)

| แหล่ง | สถานะ | หลักฐาน |
|---|---|---|
| `famoh01/ran-online-ep7` (source) | ✅ เข้าถึงได้ | clone สำเร็จ 9,982 ไฟล์, 223 MB |
| Google Drive folder `1Y3VOeAe…` | ✅ เข้าถึงได้ (12 ไฟล์) | ผ่าน connector — แต่ไฟล์ใหญ่ .00/.01 (1.57 GB + 1.01 GB) เกินเพดาน download 100 MB |
| GitHub Release `ran-ep7-full-v1` | ⚠️ เห็น metadata | 4 assets ตาม §7.4 แต่ host `release-assets.githubusercontent.com` อยู่นอก allowlist ของ sandbox |
| `matheomedrana/Ran-Online-EP7-Complete-Files` | ⚠️ เห็น metadata | repo 8.1 MB (ไฟล์ DB อยู่ใน Git LFS ⇒ ต้องดึง LFS) |
| Android SDK/NDK/Gradle/adb ในเครื่องนี้ | ❌ ไม่มี | `cmake`, `ninja`, `java`, `gradle`, `adb` = MISSING |

---

## 2. แผนผังระดับบนสุด (ของจริงใน repo)

```
famoh01/ran-online-ep7  (main @ 47f3c6d)  — 9,982 files / 1,380 .cpp / 3,798 .h
│
├── [CLIENT — Windows]  รากที่ Smtm.sln → Basic.vcxproj (MFC shell)
│   ├── Basic/                 หน้าต่าง Win32/MFC + เลือก region (EMSERVICE_*)
│   ├── RanClientLib/          เกม client logic 140 cpp · stage: DxGlobalStage / DxLobyStage / DxGameStage
│   │   └── G-Logic/           109 cpp — protocol controllers (GLContrlMsg.h = message structs)
│   ├── RanClientUILib/        239 cpp — UI ทั้งหมด (237 ไฟล์ใน Interface/)
│   ├── enginelib/             เอนจิ้นเรนเดอร์ DirectX9 (386 cpp)
│   │   ├── DxCommon9/, dxframe/   D3D9 device / DXUT framework
│   │   ├── Meshs/, DxOctree/, NaviMesh/, DxEffect/, TextTexture/, DxCommon/
│   │   └── G-Logic/
│   ├── EngineUILib/           UI framework (GUInterface: control, editbox, font)
│   ├── EngineSoundLib/DxSound/  DirectSound audio
│   ├── netclientLib/          ★ network client + PROTOCOL GROUND TRUTH (s_NetGlobal.h)
│   └── (third-party)          =boost (61 MB), =DX_Lib, =ZLIB, =TbbLib, =MAX*.SDK …
│
├── [SERVER — Windows]  Win32 / Winsock / IOCP / ODBC(MSSQL)
│   ├── LoginServer/           auth (net_LoginServer.cpp)
│   ├── SessionServer/         คั่นกลาง Login ↔ Agent
│   ├── AgentServer/           lobby/character select
│   ├── FieldServer/  GameServer/   world / zone
│   ├── BoardServer/ · ServerManager/ (25 cpp) · PatchPrimeMan/ · ServerController/
│   ├── netserverLib/          ★ IOCP + protocol + minTea (ฝั่ง server)
│   ├── netclientLib/          ★ แชร์กับ client — ตัวเดียวกัน
│   └── (GM tools)             ServerManager, PatchPrimeMan, AutoPatchMan, VerMan, VersionManager
│
├── [SHARED]  netclientLib/s_NetGlobal.h  ← client + server + mobile ต้องตรงกัน 100%
│
└── [MOBILE — งานเดิม] RANMobile/  ★ 4,434 LOC C++17
    ├── Engine/{Core,Renderer,Resource,Input→Touch,Camera,Animation,Audio,Network}
    ├── Client/{Login,Character,Gameplay,Combat,Inventory,Skills,Quest,Chat,Map}
    ├── UI/{Login,HUD,Buttons,Joystick,Inventory,Skills,Quest,Map,Chat,Settings}
    ├── Data/{Importer,Converter,Validator,Loader}
    ├── Android/{AndroidManifest.xml, native_bridge.cpp, build.gradle*, CMakeLists…}
    ├── Server/MobileServerHost.*   (host จำลองไว้ทดสอบ protocol)
    └── Tests/test_main.cpp         (15 test suites)
```

### ตัวเลขยืนยัน

| ตัววัด | ค่า |
|---|---|
| ไฟล์ทั้งหมด / `.cpp` / `.h` | 9,982 / 1,380 / 3,798 |
| MSBuild projects (`.vcxproj`) | 40 |
| Solutions | `Smtm.sln`, `Smtm2022.sln` |
| `RanClientUILib/Interface/*.cpp` | 237 |
| `RanClientLib` cpp / `G-Logic` cpp | 140 / 109 |
| `enginelib` cpp | 386 |
| `RANMobile` LOC (ไม่รวม `stb_truetype.h`) | 4,434 (Engine 1,417 · UI 885 · Data 835 · Client 556 · Tests 308 · Android 235 · Server 163) |

---

## 3. เทคโนโลยีที่ใช้ (PC) → สิ่งที่ต้องแทนบน Mobile

| ชั้น | เทคโนโลยี PC (ยืนยันจากซอร์ส) | หลักฐาน | ฝั่ง Mobile |
|---|---|---|---|
| กราฟิก | **Direct3D 9** (`IDirect3DDevice9`, `Direct3DCreate9`, DXUT) | `enginelib/DxCommon9/DXUT.h`, `enginelib/dxframe/d3dapp.cpp` | **OpenGL ES 3.0** (`GLES3/gl3.h`) — มี `MobileRenderer` แล้ว |
| Shader | D3D9 shader model / fixed function + `.fx` | `enginelib/DxEffect/` | GLSL ES 3.0 (ต้องเขียนใหม่) |
| Input | **DirectInput** (`dinput.h`) + Win32 message pump | `enginelib/DxCommon/DxInputDevice.h` | **Touch** — มี `TouchSystem` แล้ว |
| เสียง | **DirectSound** (`EngineSoundLib/DxSound/`) + Vorbis (`Vorbislib/`) | `DxSoundMan.cpp`, `BgmSound.cpp` | **OpenSL ES / AAudio** (มี `AudioManager` แบบ stub) |
| เครือข่าย | **Winsock + IOCP**, TEA encrypt, `NET_MSG_GENERIC` framing | `netclientLib/`, `netserverLib/` | **POSIX socket** (มี `NetworkClient` แล้ว แต่ protocol ผิด — §7) |
| UI/หน้าต่าง | **MFC** (`CWinApp`, `CBasicWnd`) + UI framework ของเกม | `Basic/Basic.h`, `EngineUILib/GUInterface/` | Immediate-mode quad/text (มี `DrawUIQuad`/`DrawUIText` + stb_truetype) |
| ฟอนต์/ข้อความ | `DxFontMan`, `D3DFontPar`, `GLogicText` | `EngineLib/DxCommon/DxFontMan.h` | stb_truetype + Thai glyph (มีแล้ว) |
| Script/ลอจิกที่แชร์ | `RanLogic/` (ไม่พบใน repo นี้), `G-Logic` | `RanClientLib/G-Logic/`, `enginelib/G-Logic/` | ต้อง conform โดยอ่าน struct จาก header เท่านั้น |
| Build | MSBuild / VS2022 (`Directory.Build.props`) | `.vcxproj`, `.sln` | **CMake 3.22 + NDK + Gradle 8** (มีแล้ว) |

---

## 4. Client ↔ Server: จุดแบ่งและสัญญาระหว่างกัน

**ไม่ได้แยกเป็น 2 repo/2 binary ตระกูลภาษา** — เป็น solution เดียวที่ build ออกมาเป็น `.exe` หลายตัว
จุดเชื่อมคือ **header เดียว**: `netclientLib/s_NetGlobal.h` (ใช้ทั้ง `netclientLib` และ `netserverLib`)

```
[PC Client]                                    [Server]
Basic.exe ─ RanClientLib ─ RanClientUILib      LoginServer.exe ─┐
              │   ▲                            SessionServer.exe ─┤
              │   └─ enginelib (D3D9)         AgentServer.exe  ─┼─ netserverLib
              ▼                                FieldServer.exe  ─┤   (IOCP+ODBC)
          netclientLib  ──── TCP ────►  netclientLib protocol ◄──┘
              │                                │
              └────── s_NetGlobal.h (struct + opcode) ──────┘
                            ▲
                   [RANMobile] ต้องตรงกับตัวนี้ 100%
```

### ลำดับ hop ของ Login (จากซอร์สจริง)

```
Client ──THAI_NET_MSG_LOGIN──► AgentServer ──► LoginServer(ODBC/MSSQL) ──► AgentServer ──► Client
                                                            (NET_MSG_LOGIN_FB)
```

### Opcode ที่ยืนยันแล้วว่าตรง (`EP7_NET_MSG` ที่ไม่ขึ้นกับ region)

```cpp
// netclientLib/s_NetGlobal.h:682-683  — ไม่ผูกกับ NET_MSG_BASE
NET_MSG_HEARTBEAT_CLIENT_REQ = 160;   // SERVER -> CLIENT
NET_MSG_HEARTBEAT_CLIENT_ANS = 161;   // CLIENT -> SERVER
```

### Opcode ที่ **ขึ้นกับ region** (ต้องคำนวณจาก `NET_MSG_BASE`)

```cpp
// s_NetGlobal.h:602-646  — NET_MSG_BASE เปลี่ยนตาม _PARAM macro
CH 1019 · HK 1007 · ID 977 · JP 1031 · KR 987 · KRT 971 · MY 969 · MYE 1005
PH 1017 · VN 1021 · TW 997 · TH 989 · GS 1004 · (default) 995

#define NET_MSG_LOBBY  (NET_MSG_BASE + 1013)
NET_MSG_LOGIN_2     = NET_MSG_LOBBY + 107   // Taiwan / Hong Kong path
THAI_NET_MSG_LOGIN  = NET_MSG_LOBBY + 140   // Thailand path  ← ตัวที่ build ไทยใช้จริง
```

> **สำหรับ TH_PARAM (989)**: `NET_MSG_LOBBY = 2002`, `NET_MSG_LOGIN_2 = 2109`, **`THAI_NET_MSG_LOGIN = 2142`**
> ตัวเลข `1120` ที่ mobile hardcode อยู่ = สมมติ `NET_MSG_BASE = 0` ซึ่ง**ไม่มีอยู่จริงใน EP7**

### โครงสร้าง packet ที่ต้องตรงแบบไบต์ต่อไบต์

```cpp
// s_NetGlobal.h:2050 — header ที่ทุก packet ใช้ร่วม
struct NET_MSG_GENERIC { DWORD dwSize;  EMNET_MSG nType; };   // 8 bytes, ส่งเป็น native struct

// s_NetGlobal.h:2260 — ★ struct ที่ build ไทยใช้ (ไม่ใช่ NET_LOGIN_DATA ที่ mobile ใช้)
struct THAI_NET_LOGIN_DATA {
    NET_MSG_GENERIC nmg;              // dwSize = sizeof ทั้ง struct, nType = THAI_NET_MSG_LOGIN
    int             nChannel;
    CHAR szPassword[USR_PASS_LENGTH+1]; // 21
    CHAR szUserid  [USR_ID_LENGTH+1];   // 21
};
```

ขนาดที่ต้องได้: `8 + 4 + 21 + 21 = 54` bytes และ `nmg.dwSize` **ต้องเท่ากับ 54** เพราะ server ตรวจ:
`if (sizeof(THAI_NET_LOGIN_DATA) != pNml->nmg.dwSize) → "Wrong Message Size"` (`netserverLib/s_CAgentServerMsgLogin.cpp:560`)

**การเข้ารหัส (ห้ามข้าม)**: `ThaiSndLogin` เรียก `m_Tea.encrypt()` กับ `szUserid` และ `szPassword` **全长 21 ไบต์** ก่อนส่ง
คีย์ TEA = `"Steven Seagal Neck Break"` → ตัด/ทำซ้ำให้ได้ `TEA_KEY_LENGTH = 16` ไบต์
(`netserverLib/minTea.cpp:20`, `minTea.h:257`) — server ถอดด้วยคีย์เดียวกัน (`s_CAgentServerMsgLogin.cpp:51`)

---

## 5. ไฟล์สำคัญ: Login / Character Select / Main UI

### 5.1 ฝั่ง PC (ต้นฉบับ — ห้ามแก้)

| หน้าจอ | ไฟล์ | บรรทัด | หมายเหตุ |
|---|---|---|---|
| **Login** | `RanClientUILib/Interface/LoginPage.cpp` | 419 | ★ `switch (RANPARAM::emSERVICE_TYPE)` → `ThaiSndLogin(...)` ที่บรรทัด 228 |
| | `RanClientUILib/Interface/LoginPage.h` | 54 | |
| | `netclientLib/s_NetClientMsgLogin.cpp` | — | ★ `CNetClient::ThaiSndLogin` บรรทัด 282-314 |
| Server select | `RanClientUILib/Interface/SelectServerPage.cpp` | 461 | |
| | `RanClientUILib/Interface/SelectServerChannel.cpp` | — | |
| **Character select** | `RanClientUILib/Interface/SelectCharacterInfoPage.cpp` | 171 | |
| | `RanClientUILib/Interface/SelectCharacterCard.cpp` · `SelectCharacterLeftPage.cpp` | — | |
| | `RanClientUILib/Interface/CharacterWindow.cpp` (+`Msg`,`Update`,`Ex`) | 345 | |
| สร้างตัวละคร | `NewCreateCharPage.cpp` · `CreateCharacterPage.cpp` · `CreateCharacterWeb.cpp` | — | มี web-based path ด้วย |
| **Main UI (HUD)** | `RanClientUILib/Interface/OuterInterface.cpp` | 583 | |
| | `RanClientUILib/Interface/InnerInterface.cpp` | 1,298 | |
| Stage controller | `RanClientLib/DxLobyStage.cpp` (1,423) · `DxGameStage.cpp` (801) · `DxGlobalStage.cpp` | — | ตัวกำหนดว่า state ไหนโชว์อะไร |
| Region/บริการ | `Basic/Basic.cpp:68-101` | — | `TH_PARAM → EMSERVICE_THAILAND` |

### 5.2 ฝั่ง Mobile (งานเดิม — จุดที่ต้องทำงานต่อ)

| หน้าจอ | ไฟล์ | สถานะ |
|---|---|---|
| Login | `RANMobile/UI/Login/LoginScreen.cpp` (223) + `.h` (85) | มี UI + ปุ่ม + focus/text input แต่ hit-test เป็น **พิกัด hardcode 1280×720** และ auth ยังผิด protocol |
| HUD | `RANMobile/UI/HUD/MobileHUD.cpp` | มีปุ่มครบ (Attack/Target/Skill1-9/Potion/Jump/Pickup) |
| ปุ่ม | `RANMobile/UI/Buttons/MobileButton.cpp` (53) | มี press state แต่**ไม่มี animation/scale feedback** ตามที่ Phase 5 ต้องการ |
| Joystick | `RANMobile/UI/Joystick/VirtualJoystick.cpp` | multi-touch + deadzone |
| Character select | `RANMobile/Client/Character/CharacterSelectManager.cpp` | มี manager แต่**ยังไม่มีหน้าจอวาด** |
| Entry point | `RANMobile/Android/native_bridge.cpp` (235) | `android_main` + EGL + input + **ไม่มีไฟล์ Java/Kotlin เลย (0 ไฟล์)** |

---

## 6. ช่องว่างสถาปัตยกรรมที่ต้องปิด (Gap Analysis)

| # | ช่องว่าง | ผลกระทบ | Phase |
|---|---|---|---|
| G1 | `EP7_NET_BASE` ผิด + ใช้ `NET_LOGIN_DATA` แทน `THAI_NET_LOGIN_DATA` + ไม่มี TEA | **login ไม่ผ่าน server จริง 100%** | 3.4 |
| G2 | `AuthenticationManager` ตัด 8 ไบต์ header ออกแล้วให้ `SendPacket` ใส่ใหม่ ⇒ `dwSize` ในผลลัพธ์ = 54 ✓ แต่ field order/ขนาดยังผิดตาม G1 | packet ถูกปฏิเสธ | 3.4 |
| G3 | ไม่มี mesh/map/octree renderer — `MobileRenderer` มีแค่ `DrawUIQuad`/`DrawUIText`/`UploadRGBA8Texture` | เล่นในเกมไม่ได้ (มีแต่ UI) | 3.1 |
| G4 | `VirtualJoystick`/`TouchSystem` ไม่ผูกกับ `CharacterController` → server | เดินไม่ได้ | 3.2 |
| G5 | ยังไม่มี asset pipeline รันจริงกับ `.wld/.chf/.cps/.dds/.mtf` (parser มี, decoder มี, ยังไม่ต่อเข้า renderer) | ภาพไม่ขึ้น | 3.3 |
| G6 | ไม่มี Java/Kotlin layer (ใช้ `android.app.NativeActivity` ดิบ) → คุม IME/inset/keyboard ยาก | คีย์บอร์ดบังช่องกรอก (Phase 5) | 5 |
| G7 | `RanLogic*` ไม่มีใน repo | port ฝั่ง server ได้แค่ protocol | 4 (blocker) |
| G8 | Login screen responsive: hit-box เป็นค่าคงที่ 1280×720 | กดไม่ตรงบนจอจริงหลายขนาด | 5 |
| G9 | Test suite ต้องพึ่ง asset ภายนอก → local รันไม่ได้ | ตรวจ regression ยาก | 6 |

---

## 7. ภาคผนวก: หลักฐานโปรโตคอล (ตัดตอนจากซอร์สจริง)

### 7.1 `NET_MSG_BASE` ตาม region — `netclientLib/s_NetGlobal.h:602-646`

```c
#if defined(CH_PARAM)      #define NET_MSG_BASE 1019
#elif defined(HK_PARAM)    #define NET_MSG_BASE 1007
#elif defined(ID_PARAM)    #define NET_MSG_BASE  977
#elif defined(JP_PARAM)    #define NET_MSG_BASE 1031
#elif defined(KR_PARAM)    #define NET_MSG_BASE  987
#elif defined(KRT_PARAM)   #define NET_MSG_BASE  971
#elif defined(MY_PARAM)    #define NET_MSG_BASE  969
#elif defined(MYE_PARAM)   #define NET_MSG_BASE 1005
#elif defined(PH_PARAM)    #define NET_MSG_BASE 1017
#elif defined(VN_PARAM)    #define NET_MSG_BASE 1021
#elif defined(TW_PARAM)    #define NET_MSG_BASE  997
#elif defined(TH_PARAM)    #define NET_MSG_BASE  989   ← ไทย
#elif defined(GS_PARAM)    #define NET_MSG_BASE 1004
#else                      #define NET_MSG_BASE  995
#endif
```

### 7.2 build ไหนเลือก path ไทย — `netclientLib/s_NetClientMsgLogin.cpp:282` + `LoginPage.cpp:228`

```cpp
int CNetClient::ThaiSndLogin(const char* userid, const char* userpass, int nChannel) {
    THAI_NET_LOGIN_DATA tnld;
    tnld.nChannel = nChannel;
    StringCchCopy(tnld.szUserid,   USR_ID_LENGTH+1,   userid);
    StringCchCopy(tnld.szPassword, USR_PASS_LENGTH+1, userpass);
    m_Tea.encrypt( tnld.szUserid,   USR_ID_LENGTH+1 );   // ← เข้ารหัส 21 ไบต์
    m_Tea.encrypt( tnld.szPassword, USR_PASS_LENGTH+1 );
    return Send((char*)&tnld);                            // ← ส่งทั้ง struct (dwSize=54)
}
```
```cpp
// RanClientUILib/Interface/LoginPage.cpp:225-231
switch ( RANPARAM::emSERVICE_TYPE ) {
case EMSERVICE_THAILAND: pNetClient->ThaiSndLogin( strID, strPW, nServerChannel ); break;
```
```cpp
// Basic/Basic.cpp:71-73
#ifdef TH_PARAM
    RANPARAM::emSERVICE_TYPE = EMSERVICE_THAILAND;
#endif
```

### 7.3 ฝั่ง server ตรวจอะไร — `netserverLib/s_CAgentServerMsgLogin.cpp`

```cpp
case THAI_NET_MSG_LOGIN : ThaiMsgLogin( pMsg ); break;      // s_CAgentServerMsg.cpp:66
...
m_Tea.decrypt( tnld->szUserid,   USR_ID_LENGTH+1 );         // ถอดด้วยคีย์เดียวกัน
m_Tea.decrypt( tnld->szPassword, USR_PASS_LENGTH+1 );
```
```cpp
// s_CAgentServerMsgLogin.cpp:560 (path NET_LOGIN_DATA)
if (sizeof(NET_LOGIN_DATA) != pNml->nmg.dwSize) { /* "Wrong Message Size" */ }
```

### 7.4 TEA — `netserverLib/minTea.cpp:15-23`, `minTea.h:257`

```cpp
minTea::minTea(void) {
    char* szKey = "Steven Seagal Neck Break";   // คีย์ default ที่ทั้ง client และ server ใช้
    setKey (szKey);                             // ทำซ้ำ/ตัดให้ได้ TEA_KEY_LENGTH = 16 ไบต์
}
```

### 7.5 CI ปัจจุบัน (สุขภาพงานเดิม)

`gh run list -R famoh01/ran-online-ep7` → workflow `Build RANMobile Android (ARM64) & Verify EP7 Release`
run ล่าสุด `45657604176` = **success** (3m31s) · ก่อนหน้านี้ success ต่อเนื่อง งานเดิมจึง "build ได้จริง"

---

## 8. สิ่งที่ **ไม่มี** ใน repo (ต้องขอจากผู้ใช้)

| รายการ | สถานะ | ที่ควรมี |
|---|---|---|
| `RanLogic/`, `RanLogicClient/`, `RanLogicServer/` | ❌ ไม่มี | game logic แชร์ — กระทบ Phase 4 อย่างมาก |
| Asset จริง `.rcc/.dds/.ntk/.wld/.chf/.cps` | ❌ 0 ไฟล์ใน Git | Release `ran-ep7-full-v1` (6.5 GB) / Google Drive |
| MSSQL backup (`.bak`) `RanUser/RanGame1/RanLog/RanShop` | ❌ ไม่มี | `matheomedrana/Ran-Online-EP7-Complete-Files` (LFS) |
| Server runtime config (`.cfg/.ini`) | ❌ ไม่มี | ต้องสร้างจากค่าจริงของผู้ใช้ |
| Release keystore (`.jks`) | ❌ ไม่มี | ผู้ใช้ต้องให้ (ห้ามสร้างแทน) |
| โลโก้/พื้นหลัง login ต้นฉบับ | ❌ ไม่อยู่ใน Git | อยู่ใน `data/gui` ของ client zip |

ทั้งหมดนี้บันทึกใน **`BLOCKERS.md`** พร้อมสิ่งที่ต้องการจากผู้ใช้

---

## 9. เส้นทางสู่ Phase ถัดไป (ข้อเสนอ)

1. **Phase 2** — `PROGRESS_REVIEW.md`: ตรวจงานเดิมละเอียด (มีแล้วในรอบนี้)
2. **Phase 3.4 ก่อน 3.1** — แก้ protocol ให้ตรง `THAI_NET_LOGIN_DATA` + TEA + opcode 2142 (มี test vector ได้โดยไม่ต้องมี server)
3. **Phase 3.1/3.3** — ต่อ `EP7WorldCharParser` + `EP7TextureDecoder` เข้า renderer (ทำได้ด้วย asset ชุดเล็ก)
4. **Phase 5** — Login/HUD redesign (เริ่มได้ทันทีด้วย procedural + asset เดิมเมื่อได้ไฟล์)
5. **Phase 4** — ติด G7 (ไม่มี `RanLogic*`) → ต้องขอไฟล์จากผู้ใช้
