# ARCHITECTURE.md — RAN Online EP7 · PC → Mobile (Android) Full Port

> **Phase 1 deliverable** (สำรวจโครงสร้าง / สถาปัตยกรรมจากหลักฐานจริง)
> Repo งาน: `famoh01/FamRanEp7-mobile-009` · branch `arena/7d98df66-famranep7-mobile-009` (base `25f5493` on `main`)
> วันที่สำรวจ: 2026-11-25 · ผู้สำรวจ: Arena Agent Mode

---

## 0. สรุปผู้บริหาร

| ประเด็น | ข้อสรุป |
|---|---|
| สถานะ repo นี้ (`FamRanEp7-mobile-009`) | **โครงเปล่า** ที่ base commit (`25f5493`, README.md อย่างเดียว) แต่มี **Draft PR #1/#2/#3** ที่บรรจุงาน Phase 1-2 + RANMobile scaffold/protocol fixes ไว้แล้ว — **ต้อง continue จาก PR ที่ดีที่สุด ไม่เริ่มใหม่** |
| PR ที่เป็นฐานดีที่สุด | **PR #2** (`arena/afe528ba-famranep7-mobile-009`, 6 commits, +11826/-1, 102 files) — มี docs (ARCHITECTURE/PROGRESS_REVIEW/BLOCKERS/PROTOCOL_FIX_REPORT), RANMobile engine/UI/data/client, และ protocol fixes (Thai login 2079, TEA, 56B native ABI, TCP frame reassembly, heartbeat). PR #3 รวม docs แต่ file set ใกล้เคียง; PR #1 เก่ากว่า |
| Upstream source จริง | `famoh01/ran-online-ep7` (main @ `47f3c6d` ตอนตรวจครั้งก่อนใน PR #2) — 9,982 files / 1,380 `.cpp` / 3,798 `.h`, 40 vcxproj, 2 sln. มี `RANMobile/` ถูก merge แล้ว (PR #1,#2 merged ใน upstream) |
| สถาปัตยกรรมเดิม (PC) | **Monolithic Win32 / MFC / DirectX 9** — client/server แยกเป็น project ใน solution เดียว แชร์ protocol ผ่าน `netclientLib/s_NetGlobal.h` |
| งาน mobile เดิม | `RANMobile/` 4,434 LOC C++17 + Android NDK/GLES3, CI เขียว (APK build ได้จริง) — มี renderer/UI/data/login scaffold แต่ **ยังเล่นเกมจริงไม่ได้** (ไม่มี mesh/map render, protocol เคยผิดและถูกแก้ใน PR #2, input ยังไม่ผูก gameplay) |
| แหล่งซอร์สเพิ่มเติมที่ตรวจ | Release `ran-ep7-full-v1` (metadata เห็น 4 assets), repo สำรอง `matheomedrana/Ran-Online-EP7-Complete-Files` (เข้าถึงได้, 8.1 MB), Google Drive folder (เห็นรายการไฟล์ผ่าน PR #2 docs แต่ไฟล์ใหญ่เกินเพดาน download / บาง host อยู่นอก allowlist) |
| Blocker สำคัญ (หลัง Phase 2) | B1-B4 ถูก resolve แบบ policy/scope ตามคำตัดสินผู้ใช้ (2026-11-25); B6 ThaiPack นำเข้าแล้ว, **B6a ServerDBTools ยังดาวน์โหลดไม่ได้**; B5 keystore deferred; B7 asset-dependent tests ยัง workaround — ดู `BLOCKERS.md` |

---

## 1. ขอบเขตการสำรวจ Phase 1 (สิ่งที่ทำในรอบนี้)

### 1.1 แหล่งที่เข้าถึงได้และตรวจแล้ว

| แหล่ง | วิธีตรวจ | ผล |
|---|---|---|
| `famoh01/FamRanEp7-mobile-009` (repo งานปัจจุบัน) | `git log/branch`, `gh pr list/view`, `git fetch` PR branches, อ่านไฟล์จาก PR #2 | baseline PR #2 ถูก import เข้ามาใน branch ปัจจุบันแล้ว (commit `c765f53`, source `f506d954`); มี PR #1-#3 (draft) เป็น reference |
| `famoh01/ran-online-ep7` (repo หลัก upstream) | `gh pr list --repo`, `gh api` branch `arena/6c988b52-ran-online-ep7`, อ้างอิง ARCHITECTURE/PROGRESS_REVIEW ที่ตรวจจาก upstream ใน PR #2 | PR #1,#2 merged; branch `arena/6c988b52-ran-online-ep7` มีอยู่ (sha `5c29d1d`). Upstream มี RANMobile merge แล้ว |
| Release `ran-ep7-full-v1` (`famoh01/ran-online-ep7`) | `gh release view` | เห็น 4 assets: `RAN-EP7-Client-TH.zip.part00` (1.57 GB), `part01` (1.01 GB), `RAN-EP7-ServerDBTools.zip` (49.8 MB), `RAN-EP7-ThaiPack-FamHawaii.zip` (1.36 MB) |
| `matheomedrana/Ran-Online-EP7-Complete-Files` (repo สำรอง) | `gh repo view`, `gh api git/trees/main?recursive=1` | repo สาธารณะ 8.1 MB, มีทั้ง compiled server/tools binaries (`ServerAgent.exe`, `ServerField.exe`, `ServerLogin.exe`, `ServerSession.exe`, editors) และ source tree ใต้ `(Source Code) Ran Online Server Files and Developer Tools/` รวม DX includes; เหมาะเป็นแหล่งอ้างอิงสำรอง ไม่ใช่ source-of-truth แทน upstream |
| Google Drive folder `1Y3VOeAeBzSbaB_o9oWLN16qD29b8Ro1m` | `google_drive.list_files` + `download_file` | เข้าถึงได้; **นำเข้าแล้ว** `RAN-EP7-ThaiPack-FamHawaii.zip` (1.36MB, file id `1IL8jlmvl5PGGHdaf5nlnoZdBMhTu25Ag`) → เก็บที่ `RANMobile/Data/External/RAN-EP7-ThaiPack-FamHawaii.zip` และแตกที่ `RANMobile/Data/External/ThaiPack/` (1046 files). ServerDBTools 49.8MB ดาวน์โหลดผ่าน connector ไม่สำเร็จ (B6a) |

### 1.2 แหล่งที่เข้าถึงไม่ได้ / เข้าถึงแบบจำกัด

| แหล่ง | ข้อจำกัด | สิ่งที่ต้องการ |
|---|---|---|
| Google Drive ไฟล์ใหญ่ (`RAN-EP7-Client-TH.00.zip` 1.57 GB, `.001.zip` 1.01 GB) | เกินเพดาน download ที่ใช้งานได้จริงในบริบทก่อนหน้า; ไม่ควรดึงเข้า workspace โดยไม่ยืนยัน | ผู้ใช้ตัดสินใจว่าจะอัปโหลดเฉพาะ asset ที่จำเป็นหรือไม่ |
| GitHub Release binary assets ผ่าน `release-assets.githubusercontent.com` | host นี้อยู่นอก allowlist ที่ระบุในระบบ (ตาม PR #2) | ผู้ใช้ดาวน์โหลดแล้วอัปโหลดไฟล์ที่จำเป็น หรืออนุญาตทางเลือก |
| Android device จริง / adb / emulator | ไม่มีใน sandbox | ทดสอบบน device จริงต้องทำโดยผู้ใช้ (Phase 6) |
| Live EP7 server + MSSQL | ไม่มี | ต้องการ endpoint+บัญชีทดสอบ หรือยอมรับ offline protocol verification |

---

## 2. สถาปัตยกรรม PC ต้นฉบับ (ยืนยันจาก upstream source ตาม PR #2 audit)

> ตัวเลขส่วนนี้อ้างอิงจาก ARCHITECTURE.md ของ PR #2 ที่ตรวจจาก `famoh01/ran-online-ep7` จริง (clone + grep + compile). ผมรักษาข้อเท็จจริงเดิม ไม่กุข้อมูลเพิ่ม

### 2.1 ภาพรวม

- **Monorepo/monosolution สไตล์ Win32**: 9,982 files / 1,380 `.cpp` / 3,798 `.h`
- **40 MSBuild projects** ใน 2 solutions: `Smtm.sln`, `Smtm2022.sln`
- **Client/Server แยกเป็นคนละ project ใน solution เดียวกัน** ไม่ใช่คนละ repo/codebase
- **สัญญากลาง**: `netclientLib/s_NetGlobal.h` (shared protocol header ที่ client+server+mobile ต้องตรงกัน 100%)

### 2.2 แผนผังองค์ประกอบ

```
famoh01/ran-online-ep7 (upstream)
│
├── [CLIENT — Windows / MFC / D3D9]
│   ├── Basic/                  Win32/MFC app shell, region selection (EMSERVICE_*)
│   ├── RanClientLib/           140 cpp; stage: DxGlobalStage/DxLobyStage/DxGameStage
│   │   └── G-Logic/            109 cpp; protocol controllers (GLContrlMsg.h)
│   ├── RanClientUILib/         239 cpp; UI ทั้งหมด (237 ไฟล์ใน Interface/)
│   ├── enginelib/              DirectX9 engine (386 cpp)
│   │   ├── DxCommon9/, dxframe/  D3D9 device / DXUT
│   │   ├── Meshs/, DxOctree/, NaviMesh/, DxEffect/, TextTexture/
│   │   └── G-Logic/
│   ├── EngineUILib/            UI framework (GUInterface)
│   ├── EngineSoundLib/DxSound/ DirectSound
│   ├── netclientLib/           ★ network client + PROTOCOL GROUND TRUTH
│   └── third-party: =boost, =DX_Lib, =ZLIB, =TbbLib, =MAX*.SDK, ...
│
├── [SERVER — Windows / Winsock / IOCP / ODBC(MSSQL)]
│   ├── LoginServer/            auth
│   ├── SessionServer/          Login ↔ Agent bridge
│   ├── AgentServer/            lobby/character select
│   ├── FieldServer/, GameServer/  world/zone
│   ├── BoardServer/, ServerManager/(25 cpp), PatchPrimeMan/, ServerController/
│   ├── netserverLib/           ★ IOCP + protocol + minTea
│   └── netclientLib/           shared with client
│
├── [SHARED CONTRACT]
│   └── netclientLib/s_NetGlobal.h  ← opcode/struct/size ที่ทุกฝั่งต้องตรง
│
└── [MOBILE — งานเดิมที่ merge แล้วใน upstream และมีต่อใน repo นี้]
    └── RANMobile/ (ดู §4)
```

### 2.3 Client / Server แยกกันตรงไหน

| มิติ | Client | Server | จุดร่วม |
|---|---|---|---|
| Entry/projects | `Basic.vcxproj` (MFC), `RanClientLib`, `RanClientUILib`, `enginelib`, `netclientLib` | `LoginServer`, `SessionServer`, `AgentServer`, `FieldServer`, `GameServer`, `netserverLib` | solution เดียวกัน |
| Runtime | D3D9 render loop + Win32 message pump + DirectInput/DirectSound | IOCP network + ODBC/MSSQL + server tick | TCP packets |
| Protocol | ส่ง/รับผ่าน `netclientLib` | รับ/ส่งผ่าน `netserverLib` + `netclientLib` headers | `s_NetGlobal.h` |
| Game logic | `RanClientLib/G-Logic`, `enginelib/G-Logic` | server-side handlers ใน `*Server/`, `netserverLib` | **ไม่มี `RanLogic/`, `RanLogicClient/`, `RanLogicServer/` ใน upstream repo** (blocker B4) |

### 2.4 Tech stack PC → เป้าหมาย Mobile

| ชั้น | PC (ยืนยัน) | Mobile เป้าหมาย | สถานะใน RANMobile |
|---|---|---|---|
| Graphics | Direct3D 9 (`IDirect3DDevice9`, DXUT) | OpenGL ES 3.0 / (Vulkan ภายหลังถ้าจำเป็น) | มี `MobileRenderer` (quad/text/texture upload) แต่ **ยังไม่มี mesh/map pipeline** |
| Shader | D3D9 fixed/fx | GLSL ES 3.0 | ยังไม่ครบ |
| Input | DirectInput + Win32 msg | Touch gestures + virtual joystick/buttons | มี `TouchSystem`, `VirtualJoystick` แต่ **ยังไม่ผูก gameplay/network** |
| Audio | DirectSound + Vorbis | OpenSL ES / AAudio | `AudioManager` stub |
| Network | Winsock + IOCP, TEA, `NET_MSG_GENERIC` | POSIX socket | มี `NetworkClient` + `EP7Protocol` + `EP7Tea` + `EP7Stream` (PR #2 แก้ Thai login ABI แล้ว) |
| UI/Window | MFC + EngineUILib GUInterface | NativeActivity + in-engine UI (quad/text) | มี UI components/screens พื้นฐาน; ไม่มี Java/Kotlin layer |
| Font/Text | DxFontMan, GLogicText | stb_truetype + Thai glyph | มีใน renderer |
| Build | MSBuild / VS2022 | CMake 3.22 + NDK + Gradle 8 | มี CMake/Gradle/AndroidManifest |

---

## 3. Protocol ground truth (สิ่งที่ mobile ต้องตรง 100%)

> อ้างอิงจากการตรวจ upstream ใน PR #2/PROTOCOL_FIX_REPORT — **ห้ามเปลี่ยนค่าเหล่านี้**

| รายการ | ค่า (TH_PARAM build) | หลักฐาน upstream |
|---|---:|---|
| `NET_MSG_BASE` (TH_PARAM) | 989 | `s_NetGlobal.h` |
| `NET_MSG_LOBBY` | 1939 (`BASE+950`, active block) | `s_NetGlobal.h` |
| `THAI_NET_MSG_LOGIN` | **2079** (`LOBBY+140`) | `s_NetGlobal.h`, `LoginPage.cpp` ใช้ `ThaiSndLogin` |
| `THAI_NET_MSG_LOGIN_FB` | 2080 | derived |
| `NET_MSG_HEARTBEAT_CLIENT_REQ` | 160 (absolute) | `s_NetGlobal.h` |
| `NET_MSG_HEARTBEAT_CLIENT_ANS` | 161 (absolute) | `s_NetGlobal.h` |
| `THAI_NET_LOGIN_DATA` size | **56 bytes** (native PC ABI, alignas(4), มี tail padding 2 bytes) | B8 static analysis ใน PR #2; **ยังไม่ยืนยันด้วย live capture** |
| Field order | `nmg(8) + nChannel(4) + szPassword[21] + szUserid[21] + tailPad[2]` | password BEFORE userid |
| TEA key | `"Steven Seagal Neck Break"` → 16-byte TEA key | `minTea.cpp` |
| TEA encrypt | เข้ารหัส `szUserid[21]` และ `szPassword[21]` ก่อนส่ง | `s_NetClientMsgLogin.cpp` ThaiSndLogin |
| Framing | `NET_MSG_GENERIC { DWORD dwSize; EMNET_MSG nType; }`, `dwSize = sizeof(struct)` | `s_NetGlobal.h`, server size check |

**ข้อควรระวังที่พบในงานเดิม:** PR ก่อนหน้าเคย hardcode `NET_MSG_BASE=0` ทำให้ได้ opcode 1120 และใช้ struct/TEA ผิด — PR #2 แก้แล้วเป็น 2079 + 56B + TEA. นี่คือ P0 ที่ต้องรักษาไม่ให้ถดถอย

---

## 4. สถานะงาน Mobile เดิมใน repo นี้ (จาก Draft PR #1/#2/#3)

### 4.1 สิ่งที่มีอยู่แล้ว (ห้ามเริ่มใหม่)

| พื้นที่ | เนื้อหา | หลักฐาน |
|---|---|---|
| `RANMobile/Engine/` | Core, Renderer(GLES3), Resource, Touch, Camera, Animation, Audio, Network(EP7Protocol/Stream/Tea/NetworkClient) | PR #2 file tree |
| `RANMobile/Client/` | Login/Auth, CharacterSelectManager, Gameplay, Combat, Inventory, Quest, Chat, Map, NPC/Monster adapters | PR #2 file tree |
| `RANMobile/UI/` | LoginScreen, HUD, Buttons, Joystick, Inventory, Skills, Quest, Map, Chat, Settings, Components(UIButton/UIInputField) | PR #1+#2 |
| `RANMobile/Data/` | Importer (EP7AssetImporter, TextureDecoder, WorldCharParser, DependencyScanner), Converter, Validator, Loader (GameDataLoader, ThaiPackLoader) | PR #2 |
| `RANMobile/Android/` | AndroidManifest, native_bridge.cpp, build.gradle, CMake integration | PR #2 |
| `RANMobile/Server/` | MobileServerHost (test/mock only) | PR #2 |
| `RANMobile/Tests/` | test_main, test_protocol, test_stream | PR #2 |
| Docs | `ARCHITECTURE.md`, `PROGRESS_REVIEW.md`, `BLOCKERS.md`, `PROTOCOL_FIX_REPORT.md` | PR #2/#3 |

### 4.2 PR ที่เกี่ยวข้อง

| Repo | PR | สถานะ | สาระสำคัญ |
|---|---:|---|---|
| `famoh01/ran-online-ep7` | #1 | MERGED | RANMobileEngine/Phases 1-31 groundwork |
| `famoh01/ran-online-ep7` | #2 | MERGED | RANMobileEngine, GLES3 Thai Font, Login UI, Android APK/AAB (branch `arena/6c988b52-ran-online-ep7`) |
| `famoh01/FamRanEp7-mobile-009` | #1 | DRAFT | Phase 1-2 architecture/progress/blockers + RANMobile snapshot |
| `famoh01/FamRanEp7-mobile-009` | #2 | DRAFT | Phase 1 verified + protocol fixes (2079/TEA/56B/frame reassembly/heartbeat) + login scaffold — **ฐานที่ advance ที่สุด** |
| `famoh01/FamRanEp7-mobile-009` | #3 | DRAFT | Phase 1 consolidation baseline |

### 4.3 ความสมบูรณ์โดยสังเขป (จาก PROGRESS_REVIEW ใน PR #2)

| ระบบ | สถานะจริง | หมายเหตุ |
|---|---|---|
| Heartbeat 160/161 | ✅ code+tests | absolute opcodes |
| Thai Login 2079 + TEA + 56B | ✅ ใน PR #2 (code+offline tests) | **ยังไม่ยืนยัน live server/capture** |
| Packet framing / TCP reassembly | ✅ ใน PR #2 | |
| GLES3 UI quad/text/Thai font | ✅ พื้นฐาน | ยังไม่ใช่ full game renderer |
| Texture decode (.mtf/.dds) | ✅ | |
| World/Char parser | ✅ parse ได้ | **ยังไม่ต่อเข้า renderer** |
| Mesh/Map render | ❌ | P0 ช่องว่างใหญ่ |
| Character Select UI (วาดได้) | ❌ | มี manager, ไม่มี screen draw |
| Input → gameplay → network | ❌ | ยังไม่ผูก |
| Login UI สวยงาม/modern/mobile | ❌ (scaffold เดิม) | Phase 5 เป้าหมายหลัก |
| Server compat protocol/auth | ⚠️ บางส่วน | ต้องระวังไม่กระทบ PC client |
| Android APK CI | ✅ เคยเขียว | local SDK ไม่มี |

---

## 5. ไฟล์สำคัญ: Login / Character Select / Main UI

### 5.1 PC ต้นฉบับ (อ้างอิง — ห้ามแก้พฤติกรรมเกม)

| หน้าจอ | ไฟล์ upstream | หน้าที่ |
|---|---|---|
| Login | `RanClientUILib/Interface/LoginPage.cpp` (419), `LoginPage.h` | UI login, dispatch ตาม `EMSERVICE_THAILAND` → `ThaiSndLogin` |
| Login network | `netclientLib/s_NetClientMsgLogin.cpp` (`CNetClient::ThaiSndLogin`) | สร้าง `THAI_NET_LOGIN_DATA`, TEA encrypt, send |
| Server select | `RanClientUILib/Interface/SelectServerPage.cpp`, `SelectServerChannel.cpp` | เลือก server/channel |
| Character select | `RanClientUILib/Interface/SelectCharacterInfoPage.cpp`, `SelectCharacterCard.cpp`, `SelectCharacterLeftPage.cpp`, `CharacterWindow.cpp` | เลือก/แสดงตัวละคร |
| Create character | `NewCreateCharPage.cpp`, `CreateCharacterPage.cpp`, `CreateCharacterWeb.cpp` | สร้างตัวละคร |
| Main HUD/outer/inner | `RanClientUILib/Interface/OuterInterface.cpp`, `InnerInterface.cpp` (1298) | HUD หลัก |
| Stage flow | `RanClientLib/DxLobyStage.cpp` (1423), `DxGameStage.cpp` (801), `DxGlobalStage.cpp` | state machine ของหน้าจอ/เกม |

### 5.2 Mobile ฝั่งงานเดิม (จุดที่ต้องต่อยอด)

| หน้าจอ | ไฟล์ (ใน PR #2) | สถานะ | Gap สำหรับ Phase 5 |
|---|---|---|---|
| Login | `RANMobile/UI/Login/LoginScreen.cpp/.h` | มี form/password mask/focus/submit callback | hit-test hardcode 1280×720, ไม่มี animation transition, ไม่มี touch feedback ครบ, ไม่มี keyboard inset handling ที่ robust, ยังไม่ modern/mobile-first |
| Login auth | `RANMobile/Client/Login/AuthenticationManager.cpp/.h` | มี TEA + `EP7ThaiNetLoginData` ใน PR #2 | ต้องคงพฤติกรรม auth/protocol เดิม |
| Character select | `RANMobile/Client/Character/CharacterSelectManager.cpp/.h` | มี logic/manager | **ไม่มีหน้าจอวาด** |
| HUD | `RANMobile/UI/HUD/MobileHUD.cpp/.h` | มีปุ่มพื้นฐาน | ต้องปรับ touch target/feedback |
| Buttons | `RANMobile/UI/Buttons/MobileButton.cpp/.h`, `MobileSkillButton` | มี press state | **ไม่มี scale/fade animation** |
| Components | `RANMobile/UI/Components/UIButton.cpp/.h`, `UIInputField.cpp/.h` | มีใน PR #2 | ใช้เป็นฐาน redesign ได้ |
| Entry | `RANMobile/Android/native_bridge.cpp` | NativeActivity EGL loop | **ไม่มี Java/Kotlin** → คุม IME/insets ยาก |

---

## 6. Gap Analysis (Phase 1 → งานที่ต้องทำ)

| ID | Gap | ผลกระทบ | Phase เป้าหมาย | Blocker |
|---|---|---|---|---|
| G1 | Mobile renderer ไม่มี mesh/map/world draw pipeline | เข้าเกมไม่ได้ | 3.1/3.3 | asset/device |
| G2 | Touch/joystick ไม่ผูก character controller + network movement | เดินไม่ได้ | 3.2 | - |
| G3 | Asset pipeline ยังไม่ต่อ end-to-end เข้า renderer/runtime | ไม่เห็นฉากจริง | 3.3 | asset |
| G4 | Login/CharSelect/Main UI ยังไม่ mobile-first สวยงาม/touch-friendly | UX ไม่ผ่าน Phase 5 | 5 | asset logo/bg (ตัดสินใจแล้วใน PR #2: procedural ได้) |
| G5 | ไม่มี Java/Kotlin IME/inset control | keyboard บังช่องกรอก | 5 | อาจต้องเพิ่ม Android layer |
| G6 | `RanLogic*` ไม่มีใน upstream | server gameplay compat ทำไม่ได้เต็ม | 4 | B4 |
| G7 | ไม่มี live server/MSSQL | verify end-to-end ไม่ได้ | 3.4/4/6 | B3 |
| G8 | ไม่มี Android SDK/NDK ใน sandbox | build local ไม่ได้ | 6 | B2 (CI workaround) |
| G9 | Test suite บางส่วนพึ่ง asset ภายนอก | รันครบในเครื่องนี้ไม่ได้ | 6 | B7 workaround |

---

## 7. ข้อเสนอการเดินงานถัดไป (หลัง Phase 1)

1. **Phase 2**: ตรวจ PR #1-#3 ใน repo นี้ + PR #1-#2 ใน upstream, สรุปสิ่งที่ใช้ต่อได้/ค้าง/เสี่ยง เป็น `PROGRESS_REVIEW.md`
2. **เลือก baseline code**: นำ code จาก PR #2 (ดีที่สุด) เข้ามาเป็นฐานบน branch ปัจจุบันอย่างเป็นระเบียบ (ไม่ merge PR เอง) แล้วเปิด Draft PR ต่อเนื่อง
3. **Phase 3.4 ก่อน** (ถ้ายังไม่ปิด): ยืนยัน protocol/ABI/tests จาก PR #2 ไม่ถดถอย — งานนี้ทำได้โดยไม่ต้องมี device
4. **Phase 5 (Login UI)**: ทำ redesign แบบ procedural/vector (ตาม B1 resolution ใน PR #2) โดยรักษา auth packet/protocol เดิม
5. **Phase 3.1-3.3/4/6**: ทำตาม blocker ที่เหลือ

---


## 8. ไฟล์เสริมที่นำเข้า (Phase 2 / B6)

| ไฟล์ | ตำแหน่งใน repo | แหล่ง | ใช้ตรงจุดไหน | สถานะ |
|---|---|---|---|---|
| `RAN-EP7-ThaiPack-FamHawaii.zip` (1.36MB) | `RANMobile/Data/External/RAN-EP7-ThaiPack-FamHawaii.zip` + แตกที่ `RANMobile/Data/External/ThaiPack/` | Google Drive file `1IL8jlmvl5PGGHdaf5nlnoZdBMhTu25Ag` (fallback จาก Release เพราะ `release-assets.githubusercontent.com` ถูกบล็อก) | อ้างอิง Thai text/npctalk/gui text (`data/gui/gameword_th.txt`, `gameintext_th.txt`, `gameextext_th.txt`, `data/glogic/npctalk/*.ntk`) สำหรับงาน Thai font/rendering/text pipeline ที่ค้าง; **ไม่ใช่ font binary** ใน zip นี้ | นำเข้าแล้ว |
| `RAN-EP7-ServerDBTools.zip` (49.8MB) | *(ยังไม่นำเข้า)* | Release `ran-ep7-full-v1` / Drive `1YZdc1OqAe8rpEfBZ6j_kb9YarLZSZtrO` | แผน: ใช้เป็น **schema/reference เท่านั้น** สำหรับ Phase 4 protocol/auth DB compatibility (ห้ามรัน/ติดตั้งเครื่องมือ) | **BLOCKED (B6a)** — ดาวน์โหลดไม่ได้ใน sandbox |

## 9. หลักฐานคำสั่งที่ใช้สำรวจ (reproducible)

```bash
# Repo ปัจจุบัน
git log --oneline -5
git branch -a --show-current; git remote -v
find . -maxdepth 2 -not -path '*/.git/*' -type f
gh pr list --state all --limit 20
gh pr view 1/2/3 --json number,title,headRefName,body,files,additions,deletions,commits

# Upstream / แหล่งเพิ่มเติม
gh pr list --repo famoh01/ran-online-ep7 --state all --limit 20
gh api repos/famoh01/ran-online-ep7/branches/arena/6c988b52-ran-online-ep7
gh release view ran-ep7-full-v1 --repo famoh01/ran-online-ep7 --json tagName,name,assets
gh repo view matheomedrana/Ran-Online-EP7-Complete-Files --json name,description,diskUsage,isPrivate,defaultBranchRef
gh api repos/matheomedrana/Ran-Online-EP7-Complete-Files/git/trees/main?recursive=1

# อ่านงานเดิมจาก PR branch
git fetch origin arena/afe528ba-famranep7-mobile-009
git show FETCH_HEAD:ARCHITECTURE.md
git show FETCH_HEAD:PROGRESS_REVIEW.md
git show FETCH_HEAD:BLOCKERS.md
git show FETCH_HEAD:PROTOCOL_FIX_REPORT.md
```

---

*เอกสารนี้เป็น Phase 1 output บน branch `arena/7d98df66-famranep7-mobile-009`. ข้อเท็จจริง upstream ที่อ้างจาก PR #2 ถูกระบุว่าเป็นผลตรวจจาก upstream audit เดิม; ผมไม่ได้กุข้อมูลเกมใหม่และไม่ได้แก้ protocol/game logic.*

## STANDING POLICY (บันทึกถาวร)

- ข้อมูล Skill/Monster/Character stat ที่จะแสดงใน UI ใดๆ ต้องมาจาก **server-side data source ที่มีหลักฐานจริงเท่านั้น** ห้าม fabricate/เดาค่าเด็ดขาด
- ถ้า parse/build ข้อมูลจาก raw source ไม่สำเร็จ: ห้ามใส่ placeholder ที่ดูเหมือนข้อมูลเกมจริงแทน — ปล่อยเป็นช่องว่าง/บันทึก blocker แทน
- Protocol constants/packet structure/game logic ต้องตรงต้นฉบับ 100%

## Third-Party / Bundled Asset Attribution

| Asset | Source | License | Location | Notes |
|---|---|---|---|---|
| Noto Sans Thai (Regular) | Google Fonts / notofonts (official): `google/fonts` repo, `ofl/notosansthai/NotoSansThai[wdth,wght].ttf` instantiated to static Regular (wght=400, wdth=100) via fonttools | SIL Open Font License 1.1 (OFL) | `RANMobile/Assets/Fonts/NotoSansThai-Regular.ttf`, `RANMobile/Assets/Fonts/LICENSE-OFL.txt` | Bundled for Thai text rendering in MobileRenderer (B8). Not derived from game assets. |
