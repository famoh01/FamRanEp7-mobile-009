# PROTOCOL_FIX_REPORT.md — Phase 3.4: Protocol Compatibility Fix

> Branch: `arena/afe528ba-famranep7-mobile-009` · วันที่: 2026-11-25
> ขอบเขต: **เฉพาะ protocol/login path** (ไม่แตะ logic อื่น)
> Ground truth: `famoh01/ran-online-ep7` ซอร์สจริง (`netclientLib/s_NetGlobal.h`,
> `netclientLib/s_NetClientMsgLogin.cpp`, `netserverLib/minTea.cpp`,
> `netserverLib/s_CAgentServerMsgLogin.cpp`, `RanClientUILib/Interface/LoginPage.cpp`)

---

## 0. ⚠️ การแก้ไขตัวเลขที่เคยรายงานผิด: opcode ไทยคือ 2079 ไม่ใช่ 2142

`PROGRESS_REVIEW.md` §4 (และคำสั่ง Phase 3.4 ข้อ 2) ระบุ `THAI_NET_MSG_LOGIN = 2142`
ซึ่งคำนวณจาก `NET_MSG_LOBBY = NET_MSG_BASE + 1013` — **แต่บล็อก +1013 อยู่ใน
`/* ... */` comment (s_NetGlobal.h:637-643) ไม่ใช่โค้ดที่ active**

บล็อกที่ active จริงคือ **2007-04-11 block** (s_NetGlobal.h:655-657):
```cpp
#define NET_MSG_LOBBY  (NET_MSG_BASE + 950)   // ← ACTIVE
```
ดังนั้นค่าที่ถูกต้อง (TH_PARAM):
| ค่า | คำนวณ | ผล |
|---|---|---|
| `NET_MSG_BASE` | TH_PARAM | **989** |
| `NET_MSG_LOBBY` | 989 + 950 | **1939** |
| `THAI_NET_MSG_LOGIN` | 1939 + 140 | **2079** ← ใช้จริง |
| `THAI_NET_MSG_LOGIN_FB` | 1939 + 141 | 2080 |
| `NET_MSG_LOGIN_2` (TW/HK, อ้างอิง) | 1939 + 107 | 2046 |

ถ้า implement 2142 ตามรายงานเดิม client จะยังคุยกับ server จริงไม่ได้
(ไม่มี case ตรงกับ 2142 ใน `s_CAgentServerMsg.cpp` dispatch) — รายงานนี้จึงใช้ **2079**
ตามกฎเหล็กข้อ 2 (protocol ตรงต้นฉบับ 100%) และ `PROGRESS_REVIEW.md` §4 ต้องถือว่า
ตัวเลข 2142/2109 ในเอกสารนั้น **ยกเลิก** (struct 54B + TEA ในเอกสารนั้นถูกต้องแล้ว)

---

## 1. สิ่งที่แก้ (6 ข้อตามคำสั่ง)

### 1.1 `RANMobile/Engine/Network/EP7Protocol.h` — base อ้างอิง TH_PARAM
- `EP7_NET_MSG_BASE`: `0` → **`989`** (TH_PARAM, s_NetGlobal.h:629-630)
- `EP7_NET_MSG_LOBBY`: `BASE + 1013` → **`BASE + 950`** (= 1939, s_NetGlobal.h:656)
- เพิ่ม `EP7_THAI_NET_MSG_LOGIN = 2079`, `EP7_THAI_NET_MSG_LOGIN_FB = 2080`
- `EP7_NET_MSG_LOGIN_2` คงไว้แต่แก้เป็น **2046** (path TW/HK อ้างอิงเท่านั้น)
- Heartbeat `160/161` คงเดิม (absolute enum, s_NetGlobal.h:682-683 — ถูกต้องอยู่แล้ว)
- ลบ `EP7_USR_RAND_PASS_LENGTH` / `EP7_ENCRYPT_KEY` (ใช้เฉพาะ struct เก่าผิด path)

### 1.2 Opcode login → `THAI_NET_MSG_LOGIN` (2079)
`AuthenticationManager::Login` ส่ง `EP7_THAI_NET_MSG_LOGIN` แทน `EP7_NET_MSG_LOGIN_2`
(ตรงกับ `LoginPage.cpp:228` → `ThaiSndLogin` สำหรับ `EMSERVICE_THAILAND`)

### 1.3 Struct → `THAI_NET_LOGIN_DATA` (54 ไบต์)
`EP7ThaiNetLoginData` แทน `EP7NetLoginData`:
```
NET_MSG_GENERIC nmg (8) + nChannel int (4) + szPassword[21] + szUserid[21] = 54
```
ลำดับ **password ก่อน userid** (s_NetGlobal.h:2260), ไม่มี `szRandomPassword`/`szEnCrypt`

### 1.4 TEA encryption — `RANMobile/Engine/Network/EP7Tea.{h,cpp}` (ใหม่)
Port `netserverLib/minTea.{h,cpp}` ตรงไบต์ต่อไบต์:
- คีย์ `"Steven Seagal Neck Break"` (minTea.cpp:20) → repeat เป็น 16 ไบต์ = `"Steven Seagal Ne"`
- `encrypt(field, 21)` / `decrypt(field, 21)` ตาม `ThaiSndLogin`/`ThaiMsgLogin`
- TEA core (Wheeler & Needham, `q = floor(6 + 52/n)`, delta `0x9E3779B9`) — รวมวงเล็บ
  ชัดแจ้งเพื่อผ่าน `-Werror=parentheses` โดยไม่เปลี่ยน semantics
- Wire framing คงเดิม: PC `Send()` ส่ง raw struct `nmg.dwSize` (54) ไบต์
  (s_NetClient.cpp:827-830); mobile `SendPacket` สร้าง header `{dwSize=54, nType=2079}`
  + payload 46 ไบต์ = **wire เดียวกัน 54 ไบต์**

### 1.5 Unit test vector — `RANMobile/Tests/test_protocol.cpp` (ใหม่, ไม่ต้องมี server/asset)
รันแยกจาก `test_main.cpp` (target `ranmobile_protocol_tests` ใน CMakeLists):
```
g++ -std=c++17 -Wall -Wextra -Werror RANMobile/Tests/test_protocol.cpp \
    RANMobile/Engine/Network/EP7Tea.cpp -o protocol_tests && ./protocol_tests
```
ผลรันจริงในเครื่องนี้:
```
[PASS] TEA key derivation
[PASS] TEA encrypt/decrypt vectors (5 cases)
[PASS] Full THAI login packet roundtrip (54B, opcode 2079)
[PASS] Struct field order (password before userid)
[PASS] 20-char truncation (StringCchCopy semantics)
ALL PROTOCOL TESTS PASSED!
```

**Test vector (cross-check กับ Python transcription อิสระของ minTea.cpp):**
| plaintext (21B field) | ciphertext (hex) |
|---|---|
| `hawaii01` | `d4d551e8047f5be3` |
| `pass1234` | `8478fee9b989cc5c` |
| `a` | `07c585176eed22fc` |
| `famuser` | `7390251273a1c470` |
| `12345678901234567890` (20 ตัว) | `c037be4842806d9019f9af0aa12221ca23544f45` |

Python reference (เขียนใหม่จากซอร์ส C++ โดยตรง, ไม่ได้ copy โค้ด C++) ให้ผลตรงกับ
C++ ทั้ง 5 เคส — พิสูจน์ว่า port ตรง algorithm ต้นฉบับ ไม่ใช่แค่ roundtrip ภายในตัวเอง

### 1.6 แก้ false-positive "SOURCE VERIFIED"
- `RANMobile/Tests/test_main.cpp:115` ลบ `static_assert(... == 1120)` ออก
  แทนที่ด้วย asserts ที่ถูกต้อง (989 / 1939 / 2079 / 2046 / 160 / 161 / 54B)
- `EP7Protocol.h` เดิมอ้าง "Verified directly from s_NetGlobal.h" ทั้งที่ base=0
  ไม่มีในซอร์ส — แทนที่ด้วย comment อ้างอิงบรรทัดจริงทุกค่า

---

## 2. ไฟล์ที่แตะ (เฉพาะ protocol/login path)

| ไฟล์ | เปลี่ยนอะไร |
|---|---|
| `RANMobile/Engine/Network/EP7Protocol.h` | base/opcode/struct ตาม ground truth |
| `RANMobile/Engine/Network/EP7Tea.{h,cpp}` | **ใหม่** — TEA port จาก minTea |
| `RANMobile/Client/Login/AuthenticationManager.cpp` | สร้าง Thai packet + TEA + ส่ง 2079 |
| `RANMobile/Server/MobileServerHost.cpp` | รับ Thai packet + TEA decrypt (mirror `ThaiMsgLogin`) |
| `RANMobile/Tests/test_protocol.cpp` | **ใหม่** — offline vector suite |
| `RANMobile/Tests/test_main.cpp` | แก้ false-positive assert (เฉพาะบรรทัด 9) |
| `RANMobile/CMakeLists.txt` | เพิ่ม `EP7Tea.cpp` + target `ranmobile_protocol_tests` |
| `README.md` | คำสั่ง build manual เว้น `test_protocol.cpp` (มี main ของตัวเอง) |

**ไม่ได้แตะ:** Renderer, UI, Data, Gameplay, Network transport (`NetworkClient`
framing ถูกต้องอยู่แล้วตาม `PROGRESS_REVIEW.md` §4.4), heartbeat logic

---

## 3. ผลตรวจ

- `g++ -std=c++17 -Wall -Wextra -Werror` (ทุกไฟล์รวม `EP7Tea.cpp`): **ผ่าน 0 error 0 warning**
- `protocol_tests`: **ผ่านทั้งหมด** (5 TEA vectors + packet roundtrip + layout + truncation)
- `test_main` compile ผ่าน; รัน abort ที่ `ThaiPackLoader` assert (บรรทัด 192) —
  **pre-existing B7** (ต้องมี asset ภายนอก) ไม่เกี่ยวกับ Phase 3.4
- ยังไม่ได้ทดสอบกับ server จริง (B3 — ผู้ใช้ยอมรับ offline vector แทนแล้ว)

## 4. ข้อจำกัดที่เหลือ (ไม่บล็อก Phase 3.4)
- `nChannel` ยัง hardcode `0` (PC รับจาก server select — รอ Phase 5 UI)
- `NetworkClient::Poll` ยังไม่มี frame reassembly + heartbeat ตามรอบ server
  (งาน B ใน `PROGRESS_REVIEW.md` §6 — ทำต่อได้เลย ไม่ติด blocker)
- `MobileServerHost` เป็น host ทดสอบภายใน ไม่ใช่ EP7 server จริง


---

## 5. Phase 3.4 งาน B — Frame reassembly + heartbeat (2026-11-25)

งานนี้แยก commit จากงาน A (`6639c26`); opcode **2079** คงเดิมตามที่ผู้ใช้อนุมัติ
ไม่แก้ gameplay/auth/UI logic และไม่เริ่ม Phase 5

### 5.1 Ground truth: รอบจริงของ heartbeat

อ้างอิง clone `famoh01/ran-online-ep7`:

| ซอร์ส | พฤติกรรม active |
|---|---|
| `netserverLib/s_CServer.h:53-54` | `HEARTBEAT_TIME=120000`, `SESSION_CHECK_TIME=300000` ms |
| `s_CLoginServer.cpp:615-675` | สลับส่ง REQ / ตรวจคำตอบทุก **>120 วินาที**; request-to-request ประมาณ **240 วินาที**, ตอบก่อนรอบตรวจถัดไป (~120 วินาที) |
| `s_CAgentServer.cpp:1310-1448` | ใช้ **SESSION_CHECK_TIME=300000** ในเงื่อนไขจริง ไม่ใช้ค่า `dwUdpateTime` ที่ส่งเข้ามา; สลับส่ง/ตรวจทุก **>300 วินาที** ⇒ REQ ห่างกัน ~600 วินาที, หน้าต่างตอบ ~300 วินาที |
| `s_CAgentServer.cpp:1377-1410` | ถ้า GameGuard init+auth เปิด ใช้ GameGuard auth แทน plain heartbeat; งานนี้ไม่ปลอม/ข้าม anti-cheat |
| `netclientLib/s_NetClientMsg.cpp:83-85` | รับ opcode **160** แล้วเรียก `SndHeartbeat()` **ทันที** |
| `s_NetClientMsg.cpp:280-305` | คำตอบ opcode **161**, header + `szEnCrypt[13]`; Login/Field เป็นศูนย์, Agent เข้ารหัส mutable key ทุกครั้ง |
| `s_NetClientMsg.cpp:159-165` | `NET_MSG_SND_ENCRYPT_KEY` อัปเดต key เริ่มต้น (StringCchCopy จำกัด 12 ตัว + NUL) |
| `s_CAgentServerMsg.cpp:37-56` | decrypt/เปรียบเทียบ key เดิม แล้วเก็บ ciphertext เป็น key รอบถัดไป + SetHeartBeat |

**ผล:** ลบ timer ส่ง 161 ทุก 15 วินาทีของ mobile เดิม แทนด้วยตอบทันทีเมื่อ
ได้รับ REQ ครบเฟรม ไม่เพิ่ม client-side idle timeout ที่ไม่มีใน PC client
ค่า `deltaTime` ไม่ใช้กำหนด heartbeat อีกต่อไป ความถี่จึงขึ้นกับ server จริง

Heartbeat answer ตาม native PC ABI: header 8 + CHAR[13] + tail padding 3 = **24 ไบต์**
(ไม่ใช่ header-only 8 ไบต์แบบ mobile เดิม); padding ที่ mobile ส่งเป็นศูนย์เสมอ
`NET_ENCRYPT_KEY` มี layout เดียวกัน opcode `1939+160=2099`
ใช้ key message เป็นจุดเริ่ม Agent heartbeat path; reconnect/disconnect reset key
Field connection ใหม่ไม่มี key จึงส่งศูนย์เหมือน PC Field path

### 5.2 Reassembly และ transport

- `EP7Stream.h` (ใหม่): อ่าน raw stream โดย `dwSize` รวม header 8 ไบต์
  เก็บ partial header/body ข้าม `Poll`, แยกหลายเฟรมใน recv เดียว และเก็บ trailing partial
- จำกัด frame **8..1024 ไบต์** ตาม `NET_DATA_BUFSIZE=1024`
  (`s_NetGlobal.h:99`, `RcvMsgBuffer.cpp::getMsg`); malformed size ปิด connection
  ไม่ scan หา header ใหม่โดยเดา เพราะจะเสีย framing
- `NetworkClient::Poll`: drain recv จน EAGAIN/EWOULDBLOCK, retry EINTR,
  EOF/fatal error ⇒ disconnect + ล้าง partial/key/send state
- Callback รับ **หนึ่งเฟรมเต็ม** ต่อครั้ง ไม่ใช่จำนวนไบต์ที่ recv ได้
  ข้อมูลชี้เข้า frame ใช้ได้ระหว่าง callback เท่านั้น
- Callback disconnect/reconnect ⇒ หยุด process buffer ของ connection เก่าทันที
  ใช้ generation counter ป้องกัน OS นำ socket fd เดิมกลับมาใช้
- ส่ง reply ด้วย SendPacket; เก็บ unsent tail เมื่อ nonblocking send ได้บางส่วน/
  EAGAIN แล้ว flush ต่อใน Poll เพื่อไม่ให้ heartbeat ขาดเฟรม; EINTR retry,
  SIGPIPE ป้องกันด้วย MSG_NOSIGNAL บน Linux/Android
- SendPacket คืน true หมายถึง accepted/queued ไม่รับประกัน delivery;
  เช็ก payload size/null ก่อน enqueue; ไม่เปลี่ยน raw framing
- ไม่มี LZO decompression/garbage-envelope เพิ่มในงานนี้: scope คือ raw-struct framing
  เท่านั้น (PC มี NET_COMPRESS handler แยกต่างหาก); ไม่อ้างว่ารองรับทุก envelope แล้ว

### 5.3 ไฟล์ที่เปลี่ยนในงาน B

| ไฟล์ | เหตุผล |
|---|---|
| `Engine/Network/EP7Stream.h` | helper reassembly + request-driven heartbeat/key state ใช้ร่วมกับ tests |
| `Engine/Network/NetworkClient.{h,cpp}` | Poll integration, lifecycle reset, partial send/error handling |
| `Engine/Network/EP7Protocol.h` | เพิ่ม key-message opcode 2099; ไม่เปลี่ยนค่า login |
| `Tests/test_stream.cpp` | offline vectors + loopback transport fixture (ไม่ใช้ EP7 server จริง) |
| `CMakeLists.txt` | target แยก `ranmobile_stream_tests` |
| `README.md` | คำสั่งรวม sources เว้น test main ใหม่ |
| `BLOCKERS.md` | บันทึกการเลื่อน B2/B7 ตามคำสั่งล่าสุด |
| `PROTOCOL_FIX_REPORT.md` | รายงานงาน B ส่วนนี้ |

### 5.4 Tests และคำสั่งทำซ้ำ

```bash
g++ -std=c++17 -Wall -Wextra -Werror \
  RANMobile/Tests/test_stream.cpp \
  RANMobile/Engine/Network/NetworkClient.cpp \
  RANMobile/Engine/Network/EP7Tea.cpp \
  RANMobile/Engine/Core/Logger.cpp -o /tmp/stream_tests
 timeout 15 /tmp/stream_tests
```

ผลจริง:
- **PASS** ทุกจุด split ของเฟรม 54B (53 จุด), header/body ทีละไบต์
- **PASS** หลายเฟรมติดกัน + trailing partial, reset เมื่อ reconnect, ขนาดสูงสุด 1024B
- **PASS** reject dwSize = 0, 1, 7, 1025, 0xffffffff
- **PASS** golden heartbeat Login/Field = 24B opcode 161 + zero payload/padding
- **PASS** Agent key `hawaii01` ⇒ `d4d551e8047f5be3` (vector งาน A),
  TEA key แบบ rolling ในรอบถัดไป, malformed heartbeat/key sizes, key reset
- **PASS** เรียก Poll จริงด้วย offline loopback TCP fixture: partial header,
  ไม่มี callback ก่อนเฟรมครบ, callback ได้ 8B พอดี, reply ทันที,
  ไม่ส่ง unsolicited heartbeat แม้ deltaTime มาก, ปิดสถานะเมื่อ EOF
- **PASS** suite เดียวกันภายใต้ AddressSanitizer + UndefinedBehaviorSanitizer
- **PASS** full-source host compile `-Wall -Wextra -Werror`
  (เว้น Android bridge, dep_scan_cli และ test mains แยก)
- **PASS** protocol/TEA suite งาน A รันซ้ำ

ข้อจำกัด: ไม่ force kernel short-send/EAGAIN ใน test fixture รอบนี้;
code เก็บ unsent tail แต่ยังไม่มี deterministic fault-injection test สำหรับ branch นี้
ไม่ได้รัน asset-dependent `test_main` ซ้ำตามคำสั่ง B7 และยังไม่ทดสอบ live EP7/
Android/device (B2 เลื่อน Phase 6, B3 ยอมรับ offline แล้ว)

**สถานะ:** งาน B เสร็จในขอบเขต raw framing + plain heartbeat; หยุดรอคำสั่ง
ก่อน Phase 5 — Login UI

### 5.5 ⚠️ ข้อค้นพบแก้ความมั่นใจของรายงานงาน A — B8 (OPEN)

ระหว่างตรวจ native heartbeat ABI พบประเด็น padding ที่รายงานงาน A มองข้าม:
ผลบวก fields ของ Thai login คือ 54 แต่ **default native ABI ให้ sizeof=56**
เพราะ tail padding 2B ขณะที่งาน A ใช้ `#pragma pack(1)` ฝั่ง mobile
ไม่พบหลักฐานยืนยันว่า PC/server target ใช้ pack(1) ดังนั้นคำกล่าวเดิมว่า
54B ตรง PC 100% ยังยืนยันไม่ได้; test 54B ในงาน A ยืนยันเฉพาะ mobile layout
ไม่ใช่การ compile sizeof จาก PC target

งาน B ใช้ heartbeat/key 24B ตาม default native ABI และทดสอบสมมติฐานนี้ได้
แต่ **ยังไม่ถือว่าพิสูจน์ interoperability จริง** ดู `BLOCKERS.md` B8 สำหรับ
ผล sizeof/capture ที่ต้องการ หรือการอนุมัติแก้ ABI แยก (login 56B)
ไม่ได้เปลี่ยน login 54B ของงาน A โดยพลการ และ opcode 2079 คงเดิม

**สถานะสุดท้ายแทนข้อความสรุปก่อนหน้า:** implementation งาน B + offline tests
เสร็จสำหรับ review; การยืนยัน wire ABI ถูกบล็อกที่ B8 — หยุดรอคำตัดสิน
