# PROGRESS_REVIEW.md — RAN Online EP7 · PC → Mobile (Android)

> **Phase 2 deliverable** (ตรวจสอบงานเดิม + สถานะ baseline หลัง consolidate)
> Repo: `famoh01/FamRanEp7-mobile-009`
> Branch: `arena/7d98df66-famranep7-mobile-009` (HEAD หลัง baseline import)
> Baseline import source: PR #2 `arena/afe528ba-famranep7-mobile-009` @ `f506d9547e71f1f45eaa7eb9a26ba81948bcd644`
> วันที่รีวิว: 2026-11-25

---

## 1) สรุปผู้บริหาร

| หัวข้อ | สรุป |
|---|---|
| Baseline ที่ใช้ต่อ | **PR #2 ของ repo นี้** (`arena/afe528ba-famranep7-mobile-009`) — advance ที่สุด (protocol fix + tests + docs) ถูก import เข้ามาใน branch ปัจจุบันแล้ว |
| วิธี import | Tree checkout จาก `FETCH_HEAD` ของ PR #2 (หลีกเลี่ยง add/add conflict บน `ARCHITECTURE.md`/`BLOCKERS.md` ที่สร้างใน Phase 1 ของ branch นี้) + commit attribution ชัดเจน (`c765f53`) |
| Protocol regression | **ไม่ถดถอย** — `ranmobile_protocol_tests` และ `ranmobile_stream_tests` compile clean (`-Wall -Wextra -Werror`) และ **PASS ทั้งหมด** (ดู §3) |
| ความสมบูรณ์ภาพรวม | Engine/UI scaffold มีมาก, protocol core แก้แล้วระดับ offline, แต่ **ยังไม่ถึง playable game**: ไม่มี mesh/map renderer, input ไม่ผูก gameplay/network, character select ไม่มีหน้าจอวาด, Login UI ยังไม่ถึงเกณฑ์ Phase 5 |
| ความเสี่ยงสูงสุด | P0: mesh/map rendering pipeline ยังไม่มี; P0: protocol ยังไม่ยืนยันกับ live server จริง (offline only ตาม B3); P1: Android IME/inset + touch→gameplay binding |

---

## 2) Baseline Consolidation (Step 1)

### 2.1 สิ่งที่นำเข้า

- นำเข้าไฟล์จาก PR #2 ทั้งหมด **109 files, 11,266 insertions** (commit `c765f53`)
- เก็บ `ARCHITECTURE.md`, `BLOCKERS.md` จาก Phase 1 ของ branch นี้ไว้ (จะอัปเดต Step 3/4) เพื่อหลีกเลี่ยง conflict และรักษาบริบท branch ปัจจุบัน
- ไฟล์จาก PR #2 ที่อยู่ใน workspace ตอนนี้: `RANMobile/` ทั้งต้น, `PROGRESS_REVIEW.md` (ไฟล์นี้เขียนทับใหม่), `PROTOCOL_FIX_REPORT.md`, `README.md`, `UI_LOGIN_REDESIGN.md`

### 2.2 ตารางผลทดสอบ Protocol (no regression)

Build env: `g++ (Debian 12.2.0-14) 12.2.0`, `-std=c++17 -O2 -Wall -Wextra -Werror -lpthread`, no cmake required.

| Test binary | Source | ครอบคลุม | ผล | หมายเหตุ |
|---|---|---|---:|---|
| `ranmobile_protocol_tests` | `Tests/test_protocol.cpp` | TEA key derivation; TEA encrypt/decrypt (5 cases); full THAI login packet roundtrip (56B, opcode 2079); struct field order (password before userid); 20-char truncation (StringCchCopy semantics) | **PASS** | Offline vectors; ไม่ต้องมี asset/server |
| `ranmobile_stream_tests` | `Tests/test_stream.cpp` | TCP frame reassembly (split/bytewise/coalesced/trailing partial/invalid sizes/reset/1024B); heartbeat golden wire; agent key/rolling TEA; malformed sizes; reset; actual Poll fragmentation/callback size/immediate reply/no timer/EOF | **PASS** | Offline; รวม heartbeat/reassembly ตามที่ master command ระบุ |

**สรุปยืนยัน:** 2079 / TEA / 56B / reassembly / heartbeat = **ผ่านทั้งหมด, ไม่ถดถอย** จาก baseline PR #2

### 2.3 ข้อจำกัดการทดสอบในรอบนี้

- ไม่ได้รัน `ranmobile_tests` (test_main.cpp) แบบเต็ม เพราะมี asset-dependent tests ที่เคย abort เมื่อไม่มี `external/ep7_release/thaipack_original` (B7 workaround ที่รับทราบไว้เดิม) — ไม่ได้แก้ test_main ในรอบนี้
- ไม่มี Android SDK/NDK ใน sandbox (B2) — ไม่ได้ build APK
- ไม่มี live EP7 server (B3) — ไม่ได้ยืนยัน interoperability จริง

---

## 3) สถานะไฟล์/โมดูลใน `RANMobile/` (ประเมิน % พร้อมเหตุผล)

> เกณฑ์ประเมิน: 0% ไม่มี, 25% มีโครง/สเกเลตัน, 50% ทำงานบางส่วน offline, 75% ใกล้ใช้งานจริงแต่ยังมี gap สำคัญ, 100% พร้อมใช้งานจริงครบ flow (ต้องผ่าน device/server ที่เกี่ยวข้อง)

| โมดูล | ไฟล์หลัก | ประเมิน | เหตุผล | Gap สำคัญ |
|---|---|---:|---|---|
| **Engine/Core** | `Logger`, `RANMobileEngine` | 55% | มี engine loop/state/logging พื้นฐาน; ยังไม่ใช่ game loop ที่เชื่อมทุก subsystem ครบ | ผูก scene/stage, lifecycle จริง |
| **Engine/Renderer (GLES3)** | `MobileRenderer`, `stb_truetype.h`, `GraphicsSettings` | 45% | มี quad/text/texture upload/frustum box; มี Thai font path; ไม่มี mesh/skinned mesh/map/octree draw | mesh pipeline, material/shader, map/world render (P0) |
| **Engine/Resource** | `ResourceManager`, `ObjectPool` | 45% | มีโครง resource cache/pool; ยังไม่ผูก asset จริงครบ | asset streaming, ref lifetime กับ renderer |
| **Engine/Touch** | `TouchSystem` | 50% | มี touch event handling พื้นฐาน | ผูก gesture → camera/gameplay/UI focus แบบครบ |
| **Engine/Camera** | `MobileCamera` | 40% | มี camera พื้นฐาน | follow/lock-on/orbit mobile behavior, collide กับ world |
| **Engine/Animation** | `AnimationManager` | 30% | โครงมี | ไม่มี skinned mesh pipeline ให้ขับจริง |
| **Engine/Audio** | `AudioManager` | 20% | stub/โครง | OpenSL/AAudio, BGM/SFX จริง (ยังไม่ใช่ priority P0) |
| **Engine/Network** | `EP7Protocol.h`, `EP7Tea`, `EP7Stream`, `NetworkClient` | 70% | Thai login opcode 2079, 56B ABI, TEA, TCP reassembly, heartbeat/rolling TEA มี tests ผ่าน; ส่ง packet ได้ | ยังไม่ยืนยัน live server (B3), ยังไม่ครอบคลุม packet ครบ gameplay flow, error/reconnect policy |
| **Client/Login** | `AuthenticationManager` | 65% | สร้าง Thai login packet ถูกตาม baseline; ผูกกับ NetworkClient | live auth flow, login FB handling ครบ, error UX |
| **Client/Character** | `CharacterSelectManager` | 35% | มี manager logic | **ไม่มี Character Select UI screen วาดจริง** |
| **Client/Gameplay** | `CharacterController` | 30% | มีโครง controller | ไม่ผูก touch/joystick/network movement |
| **Client/Combat** | `TargetManager`, `CombatController` | 25% | โครงพื้นฐาน | ยังไม่ผูกกับ server packets/input/HUD จริง |
| **Client/Inventory/Quest/Chat/Map** | Managers + adapters | 25% | โครงและ data adapters | ไม่ผูก UI/data/server จริงครบ |
| **UI/Login** | `LoginScreen` | 40% | มีหน้าจอ scaffold, input, submit callback; ใช้ procedural theme ได้บางส่วน | ไม่ผ่านเกณฑ์ Phase 5: responsive จริง, touch feedback/animation ครบ, keyboard inset robust, layout ไม่บัง IME, modern polish |
| **UI/Components** | `UIButton`, `UIInputField` | 45% | มี component พื้นฐาน | touch feedback animation, min 48dp target enforcement, focus/IME behavior |
| **UI/HUD/Joystick/Buttons** | `MobileHUD`, `VirtualJoystick`, `MobileButton`, `MobileSkillButton` | 45% | มี UI วาด/รับ touch พื้นฐาน | feedback animation, binding กับ gameplay, ขนาด/ตำแหน่ง responsive |
| **UI อื่นๆ** | `InventoryUI.h`, `SkillUI.h`, `QuestUI.cpp`, `MapUI.h`, `ChatUI.h`, `SettingsUI.h` | 20-30% | ส่วนใหญ่ header/stub/beginning | ต้องผูก data + renderer + input จริง |
| **Data/Importer+Loader** | `EP7AssetImporter`, `EP7TextureDecoder`, `EP7WorldCharParser`, `GameDataLoader`, `ThaiPackLoader`, `RANMobileDataConverter`, `DataValidator` | 50% | มี parser/decoder/loader/converter/validator โครงสำคัญ; ThaiPack นำเข้ามาแล้วใน workspace | ยังไม่ต่อเข้ากับ renderer/runtime จริง, ไม่ยืนยันกับ asset ชุดเต็ม |
| **Android** | `AndroidManifest`, `native_bridge.cpp`, gradle/CMake | 55% | มีโครง NativeActivity/GLES3/ABI arm64-v8a | ไม่มี Java/Kotlin layer สำหรับ IME/insets, ต้อง verify CI/device |
| **Tests** | `test_main`, `test_protocol`, `test_stream` | 60% | protocol/stream offline ผ่าน; test_main มีครอบคลุมกว้างแต่พึ่ง asset บางส่วน | แยก asset-dependent tests (B7), เพิ่ม regression tests สำหรับ UI layout/input logic ได้ |
| **Server (mobile mock)** | `MobileServerHost` | 30% | mock/test only | ห้ามใช้แทน EP7 server จริง; ใช้ได้แค่ dev/test |

---

## 4) เปรียบเทียบ PR / Baseline

### 4.1 Repo นี้ (`famoh01/FamRanEp7-mobile-009`)

| PR | Branch | สถานะ | จุดเด่น | จุดอ่อน/ซ้ำ | การตัดสิน |
|---:|---|---|---|---|---|
| #1 | `arena/f23bb3f3-famranep7-mobile-009` | DRAFT | มี snapshot แรกสุด, มี UI screens บางส่วน (Login/Joystick/Quest/Settings/Skill/Map headers) | ไม่มี protocol fixes ชุดหลัง (2079/TEA/56B/stream/heartbeat tests), เอกสารเก่า | ใช้เป็น reference เท่านั้น |
| #2 | `arena/afe528ba-famranep7-mobile-009` | DRAFT | **ครบสุด**: docs + RANMobile + protocol fix (2079, TEA, 56B native ABI) + frame reassembly + heartbeat + test_protocol/test_stream + UI components | ยังมี gap เกมจริง (render/input binding) เหมือนที่รีวิวไว้ | **เลือกเป็น baseline** |
| #3 | `arena/dff8b152-famranep7-mobile-009` | DRAFT | consolidate docs/baseline | file set ใกล้ PR #2; ไม่ได้เหนือกว่า PR #2 ในเชิง protocol/test ที่ตรวจพบ | ไม่ใช้แทน PR #2 |

### 4.2 Upstream (`famoh01/ran-online-ep7`)

| PR | Branch | สถานะ | สาระ | ความสัมพันธ์ |
|---:|---|---|---|---|
| #1 | `arena/83858354-ran-online-ep7` | MERGED | RAN Online EP7 — FAM HAWAII Mobile Client & Engine (Phases 1–31) | งานตั้งต้น mobile ใน upstream |
| #2 | `arena/6c988b52-ran-online-ep7` | MERGED | RANMobileEngine, GLES3 Thai Font, Login UI, Android APK/AAB | งานที่ master command ระบุให้ตรวจ; เป็นรากของงานที่ไหลมาสู่ repo นี้ |

**สรุป baseline:** ใช้ **PR #2 ของ repo นี้** เป็น code baseline ปฏิบัติจริง เพราะมี protocol fixes/tests ล่าสุด; upstream PR #1/#2 ใช้เป็น historical/reference และยืนยันว่า mobile work มีรากใน upstream แล้ว ไม่ใช่งานเริ่มใหม่

---

## 5) Gap Analysis — Phase 3.1-3.4 ที่เหลือ

| Phase | Gap | สถานะปัจจุบัน | งานที่ต้องทำ | Dependency | ความเสี่ยง |
|---|---|---|---|---|---|
| **3.1 Rendering (DX9→GLES3)** | ไม่มี mesh/skinned/model/map renderer | มี quad/text/texture upload เท่านั้น | ออกแบบ render pipeline สำหรับ static/skinned mesh, material/texture bind, camera/frustum/culling, map chunk/octree draw; เชื่อม EP7 parsers/decoders | Data parsers/decoders, asset format จริง, test assets | **สูงมาก** - เป็นคอขวดทำให้เกม "มองเห็น" ไม่ได้ |
| **3.2 Input (Mouse/KB→Touch)** | Touch/joystick ไม่ผูก gameplay | มี TouchSystem/VirtualJoystick events แต่ไม่ขับ CharacterController/network | Mapping gesture/joystick/button → movement/action/camera; ป้องกัน touch conflict; ผูกกับ network movement packets ที่ตรงโปรโตคอล | 3.1 บางส่วนเพื่อเห็นผล, protocol packets movement | **สูง** - เล่นไม่ได้แม้ render ได้ |
| **3.3 Asset pipeline** | parser/decoder ยังไม่ end-to-end กับ renderer/runtime | มี `EP7TextureDecoder`, `EP7WorldCharParser`, `EP7AssetImporter`, `ThaiPackLoader` | สร้าง asset manifest/load path, cache, texture/material binding, world/object spawn จากข้อมูลจริง; ห้ามกุข้อมูลเกม | asset ตัวอย่างที่มีสิทธิ์/อนุญาต, renderer | **สูง** - เสี่ยง memory/perf บนมือถือ; asset ต้นฉบับหายาก |
| **3.4 Network protocol layer** | Offline ถูกแล้ว แต่ยังไม่ครบ flow + ไม่ live-verified | 2079/TEA/56B/reassembly/heartbeat ผ่าน tests | เพิ่ม packet coverage ที่จำเป็นต่อ login→char select→enter world (ตามโปรโตคอลเดิมเท่านั้น), เพิ่ม regression tests, ทำ capture checklist; ไม่แต่ง packet | B3 live server/capture (ยังไม่มี) | **กลาง-สูง** - offline ผ่านแต่ live อาจเจอ ABI/flow edge cases |

### 5.1 ลำดับที่แนะนำสำหรับ Phase 3 (ข้อเสนอ)

1. **3.4 hardening (offline) + test harness** — ทำได้ทันทีใน sandbox, ลดความเสี่ยงถดถอย protocol; เพิ่ม tests เฉพาะ behavior ที่ยืนยันจาก source.
2. **3.2 input binding แบบจำกัด** — ผูก joystick/button events เข้ากับ controller state machine โดยไม่เปลี่ยน protocol payload semantics.
3. **3.3 asset pipeline แบบ vertical slice** — เลือก asset ชุดเล็กที่อนุญาต/มีอยู่ (หรือ procedural placeholder สำหรับ UI only) เพื่อพิสูจน์ decode→upload→draw path.
4. **3.1 mesh/map renderer** — งานหนักสุด; ต้องแตกเป็น PR ย่อยเล็กๆ (texture/material, static mesh, skinned mesh, map chunk).

> หมายเหตุ: ลำดับนี้เป็นข้อเสนอทางวิศวกรรมเพื่อคุมความเสี่ยง; ต้องรอคำสั่งผู้ใช้ก่อนเริ่ม Phase 3.

---

## 6) ความเสี่ยง (เรียงความสำคัญ)

| ลำดับ | ความเสี่ยง | ผลกระทบ | โอกาสเกิด | บรรเทา |
|---:|---|---|---|---|
| R1 | **ไม่มี mesh/map renderer** | เกมไม่สามารถแสดงฉาก/ตัวละครจริง | สูงมาก (ยังไม่มี) | แตกงาน 3.1 เป็น vertical slice; เริ่มจาก static mesh + texture + camera |
| R2 | **Protocol live incompatibility** แม้ offline tests ผ่าน | ต่อ server จริงไม่ได้/หลุด | กลาง | รักษา offline vectors; ทำ capture checklist; ห้ามเดา ABI เพิ่ม |
| R3 | **ไม่มี asset ต้นฉบับครบ** | ทดสอบ 3.1/3.3/5 ไม่สมจริง | สูง | ใช้ procedural/vector สำหรับ UI (B1 อนุมัติแล้ว); ขอ asset เฉพาะที่จำเป็น; ไม่ใช้ asset ไม่มีสิทธิ์ |
| R4 | **IME/keyboard inset บน Android** (ไม่มี Java/Kotlin layer) | Login/Chat พิมพ์ไม่ได้จริงบน device | กลาง-สูง | วางแผน native bridge/Insets API; ทดสอบบน device จริงโดยผู้ใช้ (B2) |
| R5 | **Touch→gameplay binding ผิด/ไม่ครบ** | ควบคุมไม่ได้ | กลาง | ทำ input state machine + unit tests สำหรับ mapping; แยก UI touch จาก world touch |
| R6 | **Performance/memory บนมือถือ** | crash/lag | กลาง | budget texture/mesh, object pool, profiling checklist ใน Phase 6 |
| R7 | **Server scope creep (B4)** | งาน Phase 4 บานปลายเกิน protocol/auth | กลาง | ยึดขอบเขตที่อนุมัติ: protocol/auth compatibility เท่านั้น |
| R8 | **Regression จากการ import baseline** | ฟีเจอร์เดิมพัง | ต่ำ (ลดแล้ว) | protocol/stream tests ผ่านแล้ว; เพิ่ม CI และแยก asset tests |

---

## 7) B6 — ไฟล์เสริมที่ดึงมาในรอบนี้

| ไฟล์ | แหล่ง | ขนาด | สถานะ | การใช้งาน | หมายเหตุ |
|---|---|---:|---|---|---|
| `RAN-EP7-ThaiPack-FamHawaii.zip` (1.36MB) | Google Drive folder `1Y3VOeAeBzSbaB_o9oWLN16qD29b8Ro1m` (file id `1IL8jlmvl5PGGHdaf5nlnoZdBMhTu25Ag`) — fallback เพราะ release-assets host ถูกบล็อก | 1,359,185 bytes | **นำเข้าแล้ว** | แตกไว้ที่ `RANMobile/Data/External/ThaiPack/` (1046 files: `data/glogic/npctalk/*.ntk`, `data/gui/gameword_th.txt`, `gameintext_th.txt`, `gameextext_th.txt`); ใช้เป็น reference สำหรับ Thai text/font/rendering work ที่ค้าง | ไม่มี .ttf/.otf ใน zip นี้; เป็น Thai text/npctalk/gui text pack |
| `RAN-EP7-ServerDBTools.zip` (49.8MB) | Release `ran-ep7-full-v1` / Drive | 49,791,515 bytes | **ยังไม่ได้นำเข้า** | เป้าหมาย: schema reference สำหรับ Phase 4 (ดูอย่างเดียว, ไม่รัน) | ดาวน์โหลดผ่าน release-assets ถูกบล็อก (EOF), `gh api` redirect ไป release-assets ก็ถูกบล็อก, google_drive connector `download_file` รายงาน "Could not retrieve..." — บันทึกใน BLOCKERS.md |

---

## 8) คำแนะนำก่อนเริ่ม Phase 3 (รอคำสั่งผู้ใช้)

**ควรเริ่ม Phase 3 จาก 3.4 hardening (offline) ก่อน** เพราะ:
- ทำได้ทันทีใน sandbox, ไม่มี blocker ใหม่
- รักษา protocol correctness ที่เป็นกฎเหล็กข้อ 2
- สร้าง regression tests เพิ่มก่อนแตะ renderer/input ที่เสี่ยงสูง

จากนั้นค่อยทำ **3.2 input binding** (vertical slice เล็กๆ) และ **3.1/3.3** แบบแบ่ง PR ย่อย โดยเฉพาะ 3.1 ต้องมีแผน asset/mesh scope ชัดเจนก่อนเริ่มเขียน renderer ใหญ่.

**ห้ามเริ่ม Phase 3/5 ในรอบนี้ตามคำสั่ง master command — หยุดรอคำสั่งถัดไป**

---

## 9) Addendum — Phase 3.4 + Phase 3.2 (2026-11-25, this round)

### 9.1 Part A — Protocol Hardening (offline)

| Test | Coverage (evidence-based) | Result |
|---|---|---:|
| `ranmobile_protocol_tests` | TEA key derivation; TEA encrypt/decrypt (5 cases); THAI login roundtrip (56B, 2079); field order; 20-char truncation | **PASS** |
| `ranmobile_stream_tests` | Frame reassembly split/bytewise/coalesced/partial/invalid/reset/1024B; heartbeat golden wire; agent key/rolling TEA; malformed sizes; Poll fragmentation/callback/EOF | **PASS** |
| `ranmobile_protocol_hardening_tests` | TEA edge cases (setKey null/empty/invalid len/all-zero/custom key roundtrip); EP7Stream min/max/invalid/recover/coalesced-3; Heartbeat 5 rolling rounds + malformed sizes + reset; NetworkClient guards (disconnected/oversize/null payload) | **PASS** |

**Notes (evidence discipline):**
- No new opcodes/structs invented. Added tests only for behaviors present in baseline code and cited upstream evidence.
- Reconnect/session timeout: **no explicit constant added** (no authoritative source evidence in mobile codebase this round). Documented as live-verify gap in `LIVE_VERIFY_CHECKLIST.md`.
- `LIVE_VERIFY_CHECKLIST.md` added with capture steps for login/heartbeat/framing/movement and reporting template.

### 9.2 Part B — Input Binding Vertical Slice (offline)

| Item | Evidence / implementation | Result |
|---|---|---:|
| Protocol constants/structs | Added `EP7_NET_MSG_GCTRL=2889`, `EP7_NET_MSG_GCTRL_GOTO=3031`, `EP7_NET_MSG_GCTRL_MOVESTATE=3029`, `EP7_EM_ACT_RUN=0x1`, `EP7SNetPCGoto` (36B), `EP7SNetPCMoveState` (12B) with source citations to `s_NetGlobal.h`, `GLContrlPcMsg.h`, `GLCharDefine.h` | **PASS** (static_assert) |
| State machine extensibility | `InputStateMachine` supports Movement/Skill/Inventory/Interact/None transitions; Movement active, Skill/Inventory placeholders only | **PASS** |
| Touch→intent→packet mapping | `MovementInputController`: joystick axis + camera yaw + pos/speed/dt → `MovementIntent` → `EP7SNetPCGoto`; deadzone/idle handled; run/walk via EM_ACT_RUN | **PASS** |
| Engine wiring | `RANMobileEngine` routes InGame touch to `MobileHUD`; Update builds GOTO packet and sends via `NetworkClient::SendPacket` (existing transport) | **Code complete (offline test covered mapping; no live send verified)** |
| Unit tests | `ranmobile_input_tests`: state machine, GOTO layout/opcode, joystick mapping, touch->joystick->intent, camera yaw rotation | **PASS** |

### 9.3 B7

- `test_main.cpp` now guards ThaiPackLoader asset checks behind `RANMOBILE_TEST_ASSETS=1` (approved 2026-11-25).
- Running `test_main` without assets reveals **pre-existing baseline failure** at LoginScreen focus/text stub (not caused by this round; Phase 5 scope).

### 9.4 Phase 3.1/3.3 readiness (preliminary)

- **Ready to start planning**, not ready to claim low-risk implementation. Foundation improved (protocol hardening + input vertical slice + GOTO struct evidence), but 3.1/3.3 remain highest risk (R1): no mesh/map renderer, no end-to-end asset->renderer path, and limited real asset availability.
- Recommend splitting 3.3 (asset decode/load vertical slice) and 3.1 (minimal static mesh/texture draw path) into small PRs with strict no-fake-data gate.

## 10) Addendum — Phase 3.1a + 3.3a Renderer Foundation (Engineering Test Assets Only, 2026-11-25)

### 10.1 สิ่งที่ทำ (Step 1)

**PR-A: Shader + Pipeline Bootstrap**
- เพิ่ม `Engine/Renderer/RenderMath.h/.cpp`: Mat4 (column-major), Identity/Multiply/Perspective/Ortho/LookAt/Translate/Scale/RotateY/TransformPoint (CPU-only)
- เพิ่ม `Engine/Renderer/ShaderPipeline.h/.cpp`: GLES3 vertex+fragment shader compile/link/error handling, MVP uniform + sampler set; shader source เป็น `ENGINEERING_TEST_ASSET` (ไม่ใช่เนื้อหาเกม). บน non-Android build คืนค่า `runtimeGLAvailable=false`/log ชัดเจน
- Unit test: `Tests/test_renderer_math.cpp` — **PASS** (CPU matrix math, ไม่ต้องพึ่ง GPU)

**PR-B: Static Draw Call with Test Texture (Engineering Test Assets)**
- เพิ่ม `Engine/Renderer/EngineeringTestAssets.h/.cpp`: procedural checkerboard RGBA8, gradient RGBA8, hardcoded unit quad, hardcoded unit cube (24 verts, 36 indices, UV 0..1) — ทุกจุดมี label `ENGINEERING_TEST_ASSET — not game content`
- เพิ่ม `Engine/Renderer/EngineeringTestRenderer.h/.cpp`: VAO/VBO/IBO upload, texture upload, 1 draw call (cube + checkerboard + MVP) ใต้ `__ANDROID__`; CPU-side layout validation helpers
- Unit test: `Tests/test_engineering_assets.cpp` — **PASS** (checkerboard/gradient generation, quad/cube vertex layout, UV range [0,1], index range, stride 20 / pos offset 0 / uv offset 12 ตาม GLES3 spec)

**ผลทดสอบรอบนี้**

| Test | Result |
|---|---:|
| `ranmobile_renderer_math_tests` | **PASS** |
| `ranmobile_engineering_assets_tests` | **PASS** |

**หมายเหตุ sandbox/GLES runtime**
- Sandbox ไม่มี GPU/GLES context สำหรับรัน shader จริง. โค้ด GLES ถูก guard ด้วย `#ifdef __ANDROID__` และ compile ผ่านบน Linux (`-Wall -Wextra -Werror`).
- สถานะ: **compile-time verified only, runtime ต้องรอ device/CI Android test ตาม B2**.

### 10.2 Step 2 (ข้อมูลสำหรับประเมิน — ยังไม่ลงมือทำ)

> ไม่ได้เขียน parser format จริงใหม่ในรอบนี้ ตามคำสั่ง

**Asset format ที่เกี่ยวข้อง (จากโค้ดที่มีอยู่ใน RANMobile/Data/Importer):**

| Format | สิ่งที่ parser ปัจจุบันทำได้ (evidence ใน repo) | ขนาดไฟล์ตัวอย่างเล็กสุดที่พอทดสอบได้ (ประเมินจาก header) | ความซับซ้อน |
|---|---|---:|---|
| `.wld` (world/land) | อ่าน 128-byte fileType + uint32 version; valid version `0x0108-0x0114` (`DxLandMan::VERSION`) | **>= 132 bytes** สำหรับ header-only parse (`EP7WorldCharParser::ParseWldHeader`) | **สูงมาก** สำหรับ full parse: world มี quad/octree, object placement, texture refs, lighting, collision ฯลฯ; header-only ต่ำ |
| `.chf` (character) | อ่าน uint32 version; รองรับ `0x0100-0x0103` | **>= 4 bytes** สำหรับ header-only parse | **สูง** สำหรับ full parse: bone/mesh/skin/material/animation refs; header-only ต่ำ |
| `.cps` (skin piece) | อ่าน uint32 version; `0 < version <= 0x00000107` (`DxSkinPiece::VERSION`) | **>= 4 bytes** สำหรับ header-only parse | **สูง** สำหรับ full parse: mesh/texture/material subset; header-only ต่ำ |
| `.dds` (texture) | `EP7TextureDecoder::DecodeDDS` รองรับ DXT1/DXT3/DXT5/uncompressed RGBA8/BGRA8 | **>= 128 bytes** (DDS header) + block data ขั้นต่ำ (เช่น 4x4 DXT1 = 8 bytes/block) => ~136+ bytes สำหรับภาพเล็กสุดแบบ valid | **กลาง**: header + block decode ชัดเจน; มีหลาย format/flags |
| `.mtf` (encrypted texture) | `DecryptMTF`: 12-byte header (version,size,fileType) + payload XOR `0x47` + diff `0x03` | **>= 13 bytes** (12 header + 1 byte payload) สำหรับ decrypt path; ใช้งานจริงต้องเป็น DDS/TGA valid หลัง decrypt | **ต่ำ-กลาง**: decrypt ง่าย; complexity อยู่ที่ payload format ภายใน |

**ข้อเสนอเพื่อ Step 2 จริง (เมื่อผู้ใช้พร้อม):**
- ขอไฟล์ตัวอย่าง **ขนาดเล็กมาก** ที่มีสิทธิ์ใช้งานสำหรับทดสอบ header/parser: 1 ไฟล์ `.wld`, 1 `.chf`, 1 `.cps`, 1 `.dds` (หรือ `.mtf`) ขนาดเล็กสุดเท่าที่หาได้. ไฟล์ header-only/small texture จะเพียงพอสำหรับเริ่ม parser regression tests โดยไม่ต้องโหลด asset ชุดเต็ม.
- ถ้าไม่สามารถส่ง asset จริงได้ ให้ยืนยันว่าจะอนุญาตสร้าง **synthetic binary fixtures** ที่ mimic เฉพาะ header/layout ตาม format spec ที่อ่านได้จาก source (ไม่ใช่เนื้อหาเกม) เพื่อใช้ใน unit tests — ต้องขออนุมัติก่อนเพราะเป็น binary fixture ใหม่.

## Addendum 10.3 — Phase 3.1a/3.3a Step 2 (Synthetic Fixtures) + Android Compile-Check CI

Date: 2026-11-25 (UTC)
Branch: `arena/7d98df66-famranep7-mobile-009`
Status: **CPU-verified synthetic fixture parser regression PASS; local `__ANDROID__` stub compile-check PASS; GitHub NDK CI workflow added (run pending on GitHub)**

### Decision (user-approved)
- Step 2 uses **synthetic binary header fixtures** (no real game assets). No download of large archives.

### Synthetic Fixture Rules Applied
- Fixtures contain only header/layout-structural bytes derived from documented/source-evidenced version/size constants.
- No monster/NPC/item/skill/quest/map names or game content. FileType placeholders use `TEST_FIXTURE_*`; payloads use sequential/zero patterns.
- Every fixture labeled `SYNTHETIC_TEST_FIXTURE — structural layout only, no game content, generated to validate parser mechanics against documented spec`.
- Fixtures: `RANMobile/Tests/Fixtures/` (`synthetic_wld_*.bin`, `synthetic_chf_*.bin`, `synthetic_cps_*.bin`, `synthetic_dds_*.bin`, `synthetic_mtf_*.bin`, `README.md`).

### Parser Regression Tests (no parser logic changes)
Test: `RANMobile/Tests/test_synthetic_fixtures.cpp` (`ranmobile_synthetic_fixture_tests`)

| Format | Valid case | Invalid cases | Extra | Result |
|---|---|---|---|---:|
| `.wld` | version `0x0108` (min supported), 132B header, placeholder fileType | version below min (`0x0107`), truncated (<132B) | fileType placeholder asserted | **PASS** |
| `.chf` | versions `0x0100-0x0103` | unsupported version (`0x9999`), truncated (<4B) | - | **PASS** |
| `.cps` | min `1`, max `0x00000107` (`DxSkinPiece::VERSION`) | zero, over max (`0x00000108`), truncated (<4B) | - | **PASS** |
| `.dds` | minimal DXT1 4x4 (128B header + 8B block = 136B), decoded RGBA8 all white | bad magic, truncated (<128B), zero dimension | structural decode pixel check | **PASS** |
| `.mtf` | decrypt known synthetic plaintext/ciphertext pair (XOR 0x47 + diff 0x03), `outFileType==7` | nSize too large, truncated (<=12B) | round-trip proof: inverse encrypt matches fixture cipher and decrypts back | **PASS** |

Sandbox run: `g++ -std=c++17 -O2 -Wall -Wextra -Werror ... -o /tmp/ranmobile_synthetic_fixture_tests && /tmp/ranmobile_synthetic_fixture_tests` → **ALL SYNTHETIC FIXTURE TESTS: PASS**.

### Android Compile-Check CI (B2 partial mitigation)
- Added workflow: `.github/workflows/android-renderer-compile-check.yml`.
- NDK: r25c via `nttld/setup-ndk@v1` (consistent with PR #1 baseline "NDK r25+").
- Scope: compile-only (`-c`) object files for `RANMobile/Engine/Renderer/*.cpp` + `RANMobile/Engine/Core/Logger.cpp` with `aarch64-linux-android24-clang++ -std=c++17 -O2 -Wall -Wextra -Werror -D__ANDROID__ -IRANMobile -I. -fPIC`; **no APK/AAB link**.
- Local sandbox sanity check: compiled all 5 `RANMobile/Engine/Renderer/*.cpp` with `g++ -D__ANDROID__` against minimal GLES/android/log stubs → **ALL_OK**. This validates Android-guarded syntax locally; it is **not** a substitute for real NDK/GLES headers.
- **GitHub Actions NDK run status:** workflow added and pushed; actual CI run result is pending on GitHub (NDK not available in this sandbox). If CI fails due to missing GLES headers/deps that should exist in NDK, record as blocker with error details.

### Not done (as ordered)
- No real game asset decode/content parsing; no .wld/.chf/.cps full mesh/map/content decode.
- No Phase 5 Login UI changes; no full APK/AAB build.
- No protocol/packet changes.

## Addendum 11.1 — Phase 5 Login UI Redesign (Modern Fantasy Luxury, Procedural/Vector)

Date: 2026-11-25 (UTC)
Branch: `arena/7d98df66-famranep7-mobile-009`
Status: **Phase 5 Step 1-3 implemented; unit tests PASS in sandbox; GLES visual runtime verification pending Android device/CI (B2).**

### Scope / constraints honored
- Procedural/vector UI only (gradient, SDF rounded rect, glow, shadow). No original game bitmap assets used.
- No logo file found in repo/ThaiPack; UI uses text placeholder with code label `// PLACEHOLDER_LOGO — awaiting official asset`.
- **No auth/protocol changes:** `LoginScreen::OnLoginPressed()` delegates only to `AuthenticationManager::Login(...)`; no opcode 2079/TEA/56B ABI/packet struct changes.
- No merge to main; Draft PR #4 updated.

### Step 1 — LoginScreen stub mismatch fix
- Root cause: `RANMobile/UI/Login/LoginScreen.cpp` was a Phase 5 handoff stub (no-op touch/focus/text/render), while `Tests/test_main.cpp` expected real focus/text/backspace/masked password/settings/exit behavior.
- Fix: implemented real LoginScreen behavior (layout-aware hit testing, focus, text input/backspace, settings/exit modals, render draw calls) and updated `test_main` sections 15/17 to layout-aware tap coords (down+up) via `LoginScreen::GetLayout()`; preserved test semantics. No protocol behavior fabricated.

### Step 2 — Design system / components / layout
- `UITheme.h`: central design tokens (bg `#0B0E14→#1A1F2E`, gold `#D4AF37→#F2D88F`, blue `#3A5FCD→#1E3A8A`, glass `rgba(255,255,255,0.06)` blur target 12px, border glow `#FFD700@40%` 1.5px + 4-8px, text `#F5F5F0/#8A8D98`, error `#FF5C5C`, success `#4ADE80`, typography 28/16/12sp, min touch 48dp, login button 56dp, radius 12-16dp).
- `MobileRenderer`: added extended GLES3 SDF rect shader path (rounded fill, vertical/horizontal gradient, border, glow, shadow) with Android `#ifdef` guards; old DrawUIQuad/DrawUIText API preserved.
- `UIButton`: reusable 4-state component (Default/Pressed/Disabled/Focus), 120-180ms transition, pressed scale 0.96, brightness -15%, disabled desaturate 70%/opacity 50%, focus ring, shadow, 48dp min target.
- `UIInputField`: floating label, gold focus glow, rounded box, password mask, UTF-8 backspace, cursor blink.
- `LoginScreen`: full-bleed gradient bg + lightweight procedural fog blobs (low-end fallback disables heavy effects), centered glass panel, title/subtitle placeholder logo text, username/password fields, login CTA (gold, full-width, 56dp), Register/Forgot text-outline actions (UI-only status message, no auth/protocol), Settings/Exit modals, fade+scale panel entrance 280ms cubic-out.

### Step 3 — Unit tests (no GLES runtime required)
New: `RANMobile/Tests/test_ui_phase5.cpp` (`ranmobile_ui_phase5_tests`).

| Test | Result |
|---|---:|
| Color/design token values match spec | **PASS** |
| Button state transitions Default/Pressed/Disabled/Focus + 48dp min + 0.96 pressed scale | **PASS** |
| Layout calculation (1280x720, portrait-ish, small-screen fallback; panel centering; button min sizes) | **PASS** |
| Input field focus/text/backspace/password mask | **PASS** |

### Regression tests (sandbox)

| Test | Result |
|---|---:|
| `ranmobile_tests` (env -u RANMOBILE_TEST_ASSETS) | **PASS** (includes LoginScreen Step 1 fix) |
| `ranmobile_protocol_tests` | **PASS** |
| `ranmobile_stream_tests` | **PASS** |
| `ranmobile_protocol_hardening_tests` | **PASS** |
| `ranmobile_input_tests` | **PASS** |
| `ranmobile_renderer_math_tests` | **PASS** |
| `ranmobile_engineering_assets_tests` | **PASS** |
| `ranmobile_synthetic_fixture_tests` | **PASS** |
| `ranmobile_ui_phase5_tests` | **PASS** |

### CI status
- Previous Step 2 push (`6a34e9a`) GitHub Actions **Android Renderer Compile Check (NDK)**: **SUCCESS** (confirmed via `gh run list`).
- Phase 5 push touches `MobileRenderer.*` so workflow will trigger again; final status must be checked on GitHub after push (reported in chat).

### Thai font status
- **B8 OPEN (non-blocking for Phase 5 logic):** no `.ttf/.otf` in repo/ThaiPack; MobileRenderer tries Android system NotoSansThai paths. Thai glyph rendering on devices lacking Thai system font is not guaranteed. Proposed fix: provide licensed Noto Sans Thai (SIL OFL) or approve download/bundle in next round.

### Not done
- No real GLES/visual runtime screenshot from sandbox (B2). No APK/AAB build. No protocol/auth/packet change. No game-content fabrication.

## Addendum 11.2 — B8 Fix: Bundled Noto Sans Thai (SIL OFL)

Date: 2026-11-25 (UTC)
Branch: `arena/7d98df66-famranep7-mobile-009`
Status: **B8 RESOLVED (font bundled, MobileRenderer priority wired, license/attribution added).**

### What changed
- Downloaded `NotoSansThai[wdth,wght].ttf` and `OFL.txt` from official Google Fonts repo: `google/fonts/ofl/notosansthai` (via GitHub API, allowed host).
- Instantiated static Regular (wght=400, wdth=100) with `fonttools` from PyPI (allowed host): `RANMobile/Assets/Fonts/NotoSansThai-Regular.ttf` (47,788 bytes).
- Added full license text: `RANMobile/Assets/Fonts/LICENSE-OFL.txt` (SIL OFL 1.1).
- `MobileRenderer::Initialize`: bundled font paths (`RANMobile/Assets/Fonts/...`, `Assets/Fonts/...`, `../Assets/Fonts/...`) added as **first priority** before system font fallbacks for both primary and Thai font loading.
- Attribution added to `ARCHITECTURE.md`.

### Verification
| Check | Result |
|---|---:|
| stb_truetype `InitFont` on bundled ttf | **PASS** (init=1, size=47788) |
| Thai glyph U+0E01 (ก) present | **PASS** (glyph index 29, adv 600) |
| MobileRenderer.cpp compile (sandbox g++) | **PASS** |
| Existing regression tests (see Phase 5 report) | **PASS** (no code-path regression; font load path only adds earlier candidates) |

### Notes
- Font is not derived from game assets. SIL OFL allows bundling with software when license text is included.
- GLES runtime glyph rendering still requires Android device/CI (B2 scope).

## Addendum 12.1 — Part A B8 (verified) + Part B Character Select + Part C Raw Source Investigation

Date: 2026-11-25 (UTC)
Branch: `arena/7d98df66-famranep7-mobile-009`
Status: **B8 already resolved/verified; Character Select UI shell implemented (tests PASS); Part C investigation completed with B9 blocker recorded (no data fabricated).**

### Part A — B8 Font Fix
- Verified present from commit `3e41adb`: `RANMobile/Assets/Fonts/NotoSansThai-Regular.ttf` (47,788B, SIL OFL) + `LICENSE-OFL.txt`; `MobileRenderer` bundled-first font load; attribution in `ARCHITECTURE.md`. No regression.

### Part B — Character Select Screen (Phase 5 UI scope extension)
- Added `RANMobile/UI/CharacterSelect/CharacterSelectScreen.h/.cpp` (procedural/vector, UITheme/UIButton/MobileRenderer primitives).
- Fields bind to `RANMobile::EP7CharacterSlot` struct (`charId`, `name`, `level`, `charClass`) from `Client/Character/CharacterSelectManager.h` — no hardcoded real game names/stats.
- Mockup/test labels only: `[SLOT_EMPTY]`, `TEST_SLOT_A`, `TEST_SLOT_B`. Class label mapping derived from source-evidenced `EMCHARCLASS` enum values in `GLCharDefine.h` (e.g. `0x0001 -> FIGHTER-M`), unknown values rendered as `CLASS-0x####`.
- Wired into `RANMobileEngine`: `CharacterSelectManager` + `CharacterSelectScreen` created/initialized; update/render/touch routed for `EngineScreenState::CharacterSelect`; UI-only transitions Back->Login, StartGame->InGame (no protocol fabrication).
- CMake target: `ranmobile_character_select_tests`.

### Part C — Raw Source Investigation (Skill/Monster/Character)

#### Scope inspected
1. `matheomedrana/Ran-Online-EP7-Complete-Files` (as requested)
2. `famoh01/ran-online-ep7` (readable original source used in this session)

#### Findings

| Source | Path(s) inspected | File type / observation | Case | Outcome |
|---|---|---|---:|---|
| Backup repo | `(Source Code).../Lib_Client/G-Logic/GLSkill.h` (and other G-Logic .h/.cpp), `EditorSkill/`, `EditorMobNpc/`, etc. | Git LFS pointers (e.g. GLSkill.h pointer 130B, actual 2958B, sha256 `b561fa4...`); source content not fetchable in sandbox | **C (blocked by LFS access in sandbox)** | Cannot parse/build/read actual content |
| Backup repo | `Ran Online Database/Ran Online Database SQL Script Files/RanGame1.sql` (100664B), `RanUser.sql` (96604B), `RanLog.sql`, `RanShop.sql` | Git LFS pointers (131/130B pointers). SQL content not readable without LFS fetch | **A/C blocked** (would likely be readable SQL text if fetched; currently inaccessible) | Cannot parse |
| Backup repo | `Ran Online Database/Ran Online Database Backup Files (.bak)` | Binary MSSQL backup files (dir present) | **C** | Requires MSSQL/restore tooling; not sandbox-readable |
| famoh01/ran-online-ep7 | `RanClientLib/G-Logic/GLSkill.h/.cpp`, `GLCrowData.h/.cpp`, `GLCharData.h/.cpp`, `GLCharDefine.h` | Readable C++ source (non-LFS). Defines structs/enums/loaders/save/csv; **no hardcoded skill/monster/character stat record arrays** observed in inspected files. Data records loaded from game data files at runtime (e.g. skill/crow/item data files managed by *Man loaders). | **B (structure only), records external** | No parseable stat record table in C++ source; no fabricated data |

#### LFS access attempts (backup repo)
- `gh api` contents returned LFS pointer text/base64.
- LFS batch API `https://github.com/matheomedrana/Ran-Online-EP7-Complete-Files.git/info/lfs/objects/batch` → **403 Forbidden** (with gh token Bearer, with Basic x-access-token, and anonymous).
- `git lfs` not installed in sandbox; `media.githubusercontent.com` not in sandbox allowlist.

#### Result
- **Parsed skill/monster/character stat records: 0** (no accessible plain-text/script hardcoded table; no fabrication).
- B9 recorded in `BLOCKERS.md` with explicit user needs.

### Tests (sandbox)

| Test | Result |
|---|---:|
| `ranmobile_tests` (`env -u RANMOBILE_TEST_ASSETS`) | **PASS** |
| `ranmobile_ui_phase5_tests` | **PASS** |
| `ranmobile_character_select_tests` | **PASS** |
| `ranmobile_protocol_tests` | **PASS** |
| `ranmobile_stream_tests` | **PASS** |
| `ranmobile_protocol_hardening_tests` | **PASS** |
| `ranmobile_input_tests` | **PASS** |
| `ranmobile_renderer_math_tests` | **PASS** |
| `ranmobile_engineering_assets_tests` | **PASS** |
| `ranmobile_synthetic_fixture_tests` | **PASS** |

### Not done / constraints
- No Skill/Monster/Character stat/name fabricated. No protocol/packet changes. No merge to main.
- Character Select is UI shell + struct binding; not wired to real server character-list packets (would require source-evidenced packet audit before wiring).
