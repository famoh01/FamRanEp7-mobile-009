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
