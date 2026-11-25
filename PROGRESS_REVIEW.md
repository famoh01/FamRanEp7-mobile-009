# PROGRESS_REVIEW.md — ตรวจสอบงานเดิม (Phase 2)

> Repo งาน: `famoh01/FamRanEp7-mobile-009` · branch `arena/f23bb3f3-famranep7-mobile-009`
> ตรวจงานเดิมจาก: `famoh01/ran-online-ep7` — PR #1, PR #2 (ทั้งคู่ MERGED), branch `arena/6c988b52-ran-online-ep7`
> วิธีตรวจ: clone ซอร์สจริง, **compile ด้วย g++ เอง**, รัน test, grep ground truth เทียบทีละ struct/opcode
> ใช้คู่กับ `ARCHITECTURE.md` (Phase 1)

---

## 1. สรุปคำตัดสิน (verdict)

| คำถาม | คำตอบ |
|---|---|
| งานเดิมมีจริงและใช้ต่อได้ไหม | **ใช่** — `RANMobile/` 4,434 LOC, compile ผ่าน `-Wall -Wextra -Werror`, CI เขียว, APK build ได้ |
| ต้องเริ่มใหม่ทั้งหมดไหม | **ไม่ต้อง** — per Master Command ข้อ "ห้ามเริ่มใหม่ทั้งหมด" เราจะต่อยอดจากของเดิม |
| งานเดิม "สมบูรณ์" ไหม | **ไม่** — engine/UI มีโครงครบ แต่ **ต่อ server จริงไม่ได้** (protocol ผิด) และ **ยัง render เกมไม่ได้** (มีแต่ UI) |
| ตัวเลขที่งานเดิมรายงานไว้ เชื่อได้ไหม | **ตัวเลข build/CI เชื่อได้** แต่ **ข้อความ "SOURCE VERIFIED / LOCAL TESTED" ของ protocol ไม่จริง** (§4) |

---

## 2. สิ่งที่งานเดิมทำสำเร็จ (ของจริง — ผ่านการตรวจ)

### 2.1 Build & CI — ✅ พิสูจน์แล้ว

| หลักฐาน | ผล |
|---|---|
| `g++ -std=c++17 -Wall -Wextra -Werror` กับ 33 ไฟล์ใน `RANMobile/` | **ผ่าน 0 error 0 warning** |
| `gh run list` run `45657604176` (workflow `Build RANMobile Android (ARM64)`) | **success** 3m31s |
| artifacts ที่งานเดิมรายงาน | `app-debug.apk`, `libranmobile-arm64-v8a` — มีจริงตาม `ARTIFACT_MANIFEST.md` |
| `RANMobile/CMakeLists.txt` | 3 targets, รองรับ Android NDK + Linux |

### 2.2 สถาปัตยกรรม mobile — ✅ มีโครงครบและสะอาด

```
RANMobile/Engine/     21 ไฟล์  1,417 LOC   MobileRenderer(GLES3) · ResourceManager · TouchSystem
                                          MobileCamera · AnimationManager · AudioManager · NetworkClient
RANMobile/UI/         17 ไฟล์    885 LOC   LoginScreen · VirtualJoystick · MobileButton · MobileSkillButton
                                          MobileHUD · QuestUI · InventoryUI · SkillUI · MapUI · ChatUI · SettingsUI
RANMobile/Data/       17 ไฟล์    835 LOC   EP7AssetImporter · EP7TextureDecoder · EP7WorldCharParser
                                          RANMobileDataConverter · DataValidator · GameDataLoader · ThaiPackLoader
RANMobile/Client/     20 ไฟล์    556 LOC   AuthenticationManager · CharacterSelectManager · CharacterController
                                          TargetManager · CombatController · InventoryManager · QuestManager
                                          MapManager · ChatManager · MonsterAdapter · NPCAdapter
RANMobile/Android/     5 ไฟล์    235 LOC   AndroidManifest.xml · native_bridge.cpp · build.gradle
RANMobile/Server/      2 ไฟล์    163 LOC   MobileServerHost (ใช้ทดสอบ protocol, ไม่ใช่ EP7 server จริง)
RANMobile/Tests/       1 ไฟล์    308 LOC   15 test suites
```

### 2.3 Rendering / Font — ✅ ใช้งานได้จริง

- `MobileRenderer` มี `DrawUIQuad`, `DrawUIText`, `MeasureTextWidth`, `UploadRGBA8Texture`, `DeleteTexture`, `IsBoxInFrustum`
- ฟอนต์: `stb_truetype.h` + `DecodeUTF8` + glyph cache + Thai font fallback (ตรงกับที่งานเดิมอ้าง)
- `EP7TextureDecoder`: ถอด `.mtf` (XOR `0x47`, diff `0x03`) และ `.dds` DXT1/DXT3/DXT5 → RGBA8

### 2.4 Data layer — ✅ ไม่มีการกุข้อมูล (ตรงกฎเหล็กข้อ 1)

- ไม่มี Monster/NPC/Item/Skill/Quest/Map ที่สร้างขึ้นเองใน `RANMobile/`
- `MonsterAdapter`/`NPCAdapter` เป็น view-only (`GetState().globId`)
- `DataValidator` + `RANMobileDataConverter` อ่านจากไฟล์ EP7 เท่านั้น

### 2.5 Android packaging — ✅ พร้อมต่อยอด

- `AndroidManifest.xml`: `minSdk` implied 24 / `targetSdk 34`, `glEsVersion 0x00030000`, `screenOrientation="landscape"`
- `app/build.gradle`: `applicationId com.famhawaii.ranmobile`, `abiFilters arm64-v8a`, `externalNativeBuild` → `RANMobile/CMakeLists.txt`

---

## 3. สิ่งที่งานเดิม **ยังไม่เสร็จ** (เรียงตามความรุนแรง)

| # | ช่องว่าง | หลักฐาน | ต้องทำใน Phase |
|---|---|---|---|
| **P0-1** | **Login protocol ผิดทั้งก้อน** — opcode, struct, การเข้ารหัส | ดู §4 ด้านล่าง | 3.4 |
| **P0-2** | **ไม่มี mesh/map renderer** — `MobileRenderer` วาดได้แค่ quad/text/upload texture ไม่มี mesh pipeline, ไม่มี octree/world loading ที่ต่อเข้า `Draw` | `Engine/Renderer/MobileRenderer.h` มีแค่ 3 draw call | 3.1 / 3.3 |
| **P0-3** | **`EP7WorldCharParser` ไม่ต่อเข้า renderer** — parser ผ่าน test แต่ไม่มีใครเรียกใช้ใน `DxGameStage`-equivalent | ไม่มี `#include EP7WorldCharParser` ใน `MobileRenderer.cpp`/`RANMobileEngine.cpp` | 3.3 |
| **P1-1** | **ไม่มีหน้าจอ Character Select** — มีแต่ `CharacterSelectManager` (logic) ไม่มี draw | ไม่มีไฟล์ `UI/CharacterSelect/*` | 5 |
| **P1-2** | **Input ไม่ผูกกับ gameplay** — `VirtualJoystick`/`TouchSystem` ไม่ได้ส่งไป `CharacterController` → network | ไม่มี call path joystick → net | 3.2 |
| **P1-3** | **ไม่มี Java/Kotlin layer (0 ไฟล์)** — ใช้ `android.app.NativeActivity` ดิบ ⇒ คุม IME, window insets, edge-to-edge ยาก | `find RANMobile/Android -name "*.java" -o -name "*.kt" | wc -l` = **0** | 5 |
| **P1-4** | **Login hit-box เป็นค่าคงที่ 1280×720** ⇒ กดไม่ตรงบนจอจริง (เช่น 2400×1080) | `LoginScreen.cpp` ใช้ `m_screenW * 0.46f` + offset คงที่ `110/170/230/300/365` | 5 |
| **P1-5** | **ปุ่มไม่มี touch animation** (scale-down/fade) ตาม Phase 5 | `MobileButton` มีแค่ `m_pressed` bool ไม่มี state ต่อเวลา | 5 |
| **P2-1** | Network รับ packet แบบไม่ครบ frame — `recv` ครั้งเดียว 4096, ตัดสินขนาดจาก 1 packet, ไม่มี partial/coalesced handling | `NetworkClient::Poll` | 3.4 |
| **P2-2** | Heartbeat ส่ง `NET_MSG_HEARTBEAT_CLIENT_ANS` (161) เองทุก 15s แต่ **ไม่มี logic ตอบ `HEARTBEAT_CLIENT_REQ` ตามรอบ server** | `NetworkClient::Poll` | 3.4 |
| **P2-3** | Test suite ต้องมี asset ภายนอก (`external/ep7_release/thaipack_original`) ⇒ **รันในเครื่องนี้ไม่ได้** (abort `test_main.cpp:186`) | รันจริงแล้ว abort | 6 |
| **P2-4** | เอกสารงานเดิมหลายฉบับลงวันที่/commit ไม่ตรงกัน (เช่น `PROJECT_STATE.md` อ้าง `f520e60`, อีกฉบับอ้าง `0e711f8`) | เทียบหัวเอกสาร | 6 |

---

## 4. ★ การตรวจสอบ Protocol — จุดที่งานเดิม "อ้างว่า verified" แต่ **ไม่จริง**

### 4.1 สิ่งที่งานเดิมประกาศไว้

```cpp
// RANMobile/Engine/Network/EP7Protocol.h  (งานเดิม)
constexpr int32_t EP7_NET_MSG_BASE  = 0;                    // ← สมมติฐานที่ไม่มีในซอร์ส
constexpr int32_t EP7_NET_MSG_LOBBY = EP7_NET_MSG_BASE + 1013;
constexpr int32_t EP7_NET_MSG_LOGIN_2 = EP7_NET_MSG_LOBBY + 107;   // = 1120
```

```cpp
// RANMobile/Tests/test_main.cpp:114-115  — test ที่ "ยืนยัน" ตัวเอง
static_assert(RANMobile::EP7_NET_MSG_HEARTBEAT_CLIENT_ANS == 161, "Verified EP7 Heartbeat ID");
static_assert(RANMobile::EP7_NET_MSG_LOGIN_2 == 1120,             "Verified EP7 Login ID");
```

และใช้ struct ผิดตัว:

```cpp
// RANMobile/Client/Login/AuthenticationManager.cpp:21-29
EP7NetLoginData loginPkt;                       // ← เลียน NET_LOGIN_DATA (TW/HK path)
std::strncpy(loginPkt.szUserid,   username.c_str(), EP7_USR_ID_LENGTH);
std::strncpy(loginPkt.szPassword, password.c_str(), EP7_USR_PASS_LENGTH);
return net->SendPacket(EP7_NET_MSG_LOGIN_2, payload, payloadSize);   // ← ไม่มี TEA
```

### 4.2 ความจริงจากซอร์ส (ground truth 4 ชั้น)

| # | ชั้น | หลักฐาน | ผล |
|---|---|---|---|
| 1 | **opcode** | `s_NetGlobal.h:602-646` → TH base = **989**; `:639` `LOBBY = base+1013` = 2002 | `LOGIN_2` = **2109** ไม่ใช่ 1120 |
| 2 | **path ที่ build ไทยใช้** | `Basic/Basic.cpp:71` `#ifdef TH_PARAM → EMSERVICE_THAILAND`; `LoginPage.cpp:228` `case EMSERVICE_THAILAND: ThaiSndLogin(...)` | ใช้ **`THAI_NET_MSG_LOGIN` = LOBBY+140 = 2142** ไม่ใช่ `LOGIN_2` |
| 3 | **struct** | `s_NetGlobal.h:2260` `THAI_NET_LOGIN_DATA{ nmg; nChannel; szPassword[21]; szUserid[21]; }` | 54 ไบต์, **ลำดับ password ก่อน userid**, **ไม่มี** `szRandomPassword`/`szEnCrypt` |
| 4 | **การเข้ารหัส** | `s_NetClientMsgLogin.cpp:305-307` `m_Tea.encrypt(szUserid,21)` / `(szPassword,21)`; server `s_CAgentServerMsgLogin.cpp:51` `m_Tea.decrypt(...)` | **ต้อง TEA** คีย์ `"Steven Seagal Neck Break"` (`minTea.cpp:20`), 16 ไบต์ |
| 5 | **server dispatch** | `s_CAgentServerMsg.cpp:66` `case THAI_NET_MSG_LOGIN: ThaiMsgLogin(pMsg)` | server ไทย **ไม่มี** handler สำหรับ 1120 |

### 4.3 ผลกระทบจริง

ถ้าเอา mobile client ไปต่อ EP7 server ไทย:
1. packet type 1120 → server **ไม่ match case ไหน** → ถูกทิ้ง/ตัดการเชื่อมต่อ
2. แม้แก้ opcode เป็น 2142 แต่ struct ยังผิด (ขนาด 62 vs 54, ลำดับ field สลับ) → `Wrong Message Size` หรืออ่านขยะ
3. แม้ struct ถูก แต่ไม่มี TEA → `m_Tea.decrypt` ได้ค่าเพี้ยน → login fail

**สรุป: `SOURCE VERIFIED` ของ protocol ในงานเดิมคือ false positive** — test ที่เขียน (`static_assert == 1120`) ยืนยัน "ค่าที่ตัวเอง hardcode" ไม่ได้ยืนยันกับซอร์ส

> หมายเหตุความเป็นธรรม: งานเดิมทำถูกในส่วน **heartbeat (160/161)** ซึ่งไม่ขึ้นกับ region — อันนั้น verified จริง

### 4.4 หลักฐานประกอบ: framing ถูกต้องแล้ว

จุดนี้ **ไม่ต้องแก้** — `NetworkClient::SendPacket` ใส่ header เองแล้ว และ server อ่าน `nmg->dwSize = sizeof(struct)`:
```cpp
// RANMobile/Engine/Network/NetworkClient.cpp:96-108
std::vector<uint8_t> packet(sizeof(EP7NetMsgGeneric) + size);
hdr->dwSize = packet.size();  hdr->nType = msgType;   // ✅ ถูก
```
```cpp
// netclientLib/s_NetClient.cpp:827-831 — PC client ส่งทั้ง struct ตาม dwSize
dwSize = nmg->dwSize;
nSndBytes = ::send(m_sClient, (const char*) m_pBuffer, (int) dwSize, 0);
```

---

## 5. ตารางสถานะตามจริง (แทนตารางใน `PROJECT_STATE.md` ของงานเดิม)

| ระบบ | SOURCE VERIFIED | CODE | LOCAL TEST (เครื่องนี้) | DEVICE | LIVE SERVER |
|---|---|---|---|---|---|
| Heartbeat 160/161 | ✅ | ✅ | ✅ (161 ตรง) | — | ❌ |
| **Thai Login (2142 + TEA)** | ✅ (ตรวจใหม่) | ❌ **ผิด** | ❌ | ❌ | ❌ |
| Packet framing (`dwSize`) | ✅ | ✅ | ✅ | — | ❌ |
| GLES3 UI quad/text | n/a | ✅ | ✅ compile | ⚠️ ตามงานเดิม | — |
| Thai font (UTF-8) | n/a | ✅ | ✅ | ⚠️ ตามงานเดิม | — |
| Texture decode (.mtf/.dds) | ✅ | ✅ | ✅ unit test | ❌ | — |
| World/Char parser (.wld/.chf/.cps) | ✅ | ✅ | ✅ unit test | ❌ (ไม่ต่อ renderer) | — |
| Mesh/Map render | — | ❌ **ไม่มี** | ❌ | ❌ | ❌ |
| Character Select UI | ✅ (PC) | ❌ **ไม่มี** | ❌ | ❌ | ❌ |
| HUD / Joystick | ✅ (PC) | ✅ | ✅ | ❌ | ❌ |
| Movement → server | ✅ (PC) | ❌ ไม่ผูก | ❌ | ❌ | ❌ |
| Server (Login/Agent/Field) | ✅ | ✅ (Win32) | ❌ build ไม่ได้บน Linux | — | ❌ |
| Android APK | n/a | ✅ | ✅ CI | ⚠️ | — |

**สรุปภาพรวม: UI + data plumbing ≈ 60% · network protocol ≈ 15% (หลังหัก login ที่ผิด) · gameplay render ≈ 0% · server compat ≈ 10%**

---

## 6. แผนดำเนินการต่อ (ปรับลำดับจาก Master Command — พร้อมเหตุผล)

Master Command สั่ง 3.1 → 3.2 → 3.3 → 3.4 แต่ผมเสนอ **สลับ 3.4 ขึ้นก่อน** เพราะ:

- 3.4 (protocol) **พิสูจน์ได้ 100% ด้วย unit test + test vector** โดยไม่ต้องมี device/server/asset
  → ได้ "หลักประกันว่าไม่แต่งข้อมูล" ที่ตรวจสอบได้จริง ตรงกฎเหล็กข้อ 2
- 3.1 (renderer) ต้องมี asset ก่อน ซึ่ง **ติด blocker** (ดู `BLOCKERS.md`)
- ทำ 3.4 ก่อน = ได้ผลลัพธ์ที่ merge ได้ทันที ไม่ต้องรอผู้ใช้

| ลำดับ | งาน | ต้องมี blocker ไหม |
|---|---|---|
| **A** | Phase 3.4 — `ThaiLoginProtocol` (opcode 2142, `THAI_NET_LOGIN_DATA` 54B, TEA) + test vector | ❌ ไม่ต้อง |
| **B** | Phase 3.4 — แก้ `NetworkClient` frame reassembly + heartbeat ตามรอบ | ❌ ไม่ต้อง |
| **C** | Phase 5 — Login UI redesign (responsive + touch feedback + keyboard inset) | ⚠️ ต้องการ asset ต้นฉบับ (§7 ของ `BLOCKERS.md`) — เริ่มด้วย procedural ได้ |
| **D** | Phase 3.1/3.3 — mesh pipeline + asset pipeline | ✅ **ติด blocker** (asset + device) |
| **E** | Phase 4 — server compat | ✅ **ติด blocker** (`RanLogic*` ไม่มีใน repo) |
| **F** | Phase 6 — build/test pipeline | ❌ ทำได้ (CI) |

---

## 7. ข้อผิดพลาดเล็กๆ ที่ควรแก้ระหว่างทาง

1. `EP7_USR_RAND_PASS_LENGTH` / `EP7_ENCRYPT_KEY` ใน `EP7Protocol.h` **ไม่ถูกใช้ใน path ไทย** → เก็บไว้เพื่อ path TW/HK ได้ แต่ต้องไม่หลุดเข้า struct ไทย
2. `MobileButton::IsClicked()` เป็น latch — ไม่มี `ConsumeClick()` ⇒ เสี่ยง click ซ้ำ (ควรมี consume semantics)
3. `LoginScreen::HandleTouchDown` รับ `m_activePointerId` แต่ไม่ใช้ตรวจ pointer ที่ขึ้น → multi-touch อาจค้าง focus
4. `MobileServerHost` ตอบ heartbeat ด้วย `HEARTBEAT_CLIENT_ANS` (161) กลับไป — ผิดทิศ (161 คือ client→server) ใช้ทดสอบภายในได้ แต่ห้ามใช้เป็นตัวอ้างอิง protocol
5. เอกสารงานเดิม: `PROJECT_STATE.md` / `PROJECT_STATUS.md` / `STEP_PROGRESS.md` อ้าง commit ต่างกัน — ควรมีเอกสารสถานะเดียว (เราใช้ `ARCHITECTURE.md` + `PROGRESS_REVIEW.md` + `BLOCKERS.md` แทน)

---

## 8. ส่งต่อ Phase 3

พร้อมเริ่ม **Phase 3.4** ทันที (งาน A) — ไม่ติด blocker
รายละเอียด blocker ทั้งหมดอยู่ใน `BLOCKERS.md` และต้องรอคำสั่งผู้ใช้ก่อนไป 3.1/4
