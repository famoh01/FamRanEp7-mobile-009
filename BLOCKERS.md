# BLOCKERS.md — RAN Online EP7 · PC → Mobile (Android)

> อัปเดตล่าสุด: 2026-11-25 · branch `arena/afe528ba-famranep7-mobile-009`
> **กฎ:** ถ้าเจอ blocker ต้องบันทึกที่นี่ + ระบุว่าต้องการอะไร + **หยุดรอคำสั่ง** (Master Command ข้อ 4)
> สถานะ: `OPEN` = ต้องรอผู้ใช้ · `WORKAROUND` = มีทางเลี่ยงที่ทำได้เองแล้ว · `RESOLVED`

---

## B1 — Asset ต้นฉบับของเกม (บล็อก Phase 3.1 / 3.3 / 5)

| | |
|---|---|
| **สถานะ** | ✅ `RESOLVED` (ผู้ใช้ตัดสินใจ 2026-11-25: **ข้าม — ไม่ใช้ asset เก่า ทำ UI ใหม่แบบ procedural/vector ทั้งหมดใน Phase 5**) |
| **กระทบ** | Phase 3.1 Rendering, 3.3 Asset pipeline, **Phase 5 UI (โลโก้/พื้นหลัง login)** |

**สิ่งที่ขาด:** ไฟล์ asset จริงของ client ไทย
`.rcc` · `data/gui/` (โลโก้ + พื้นหลังหน้า login) · `.dds` / `.mtf` (texture) · `.wld` (map) · `.chf` / `.cps` (character) · `.ntk` / `.qst` (Thai pack)

**หลักฐานว่าไม่มีใน repo:**
```bash
find /tmp/ransrc \( -name "*.rcc" -o -name "*.wld" -o -name "*.chf" -o -name "*.ntk" -o -name "*.isf" \) | wc -l
# → 0
find /tmp/ransrc -type d \( -name guiskin -o -name skinobject -o -name texture \) | wc -l
# → 0
```
อยู่ใน `RAN-EP7-Client-TH.zip` (6.5 GB) บน Release `ran-ep7-full-v1` ซึ่ง host `release-assets.githubusercontent.com` **ไม่อยู่ใน outbound allowlist** ของ sandbox

**ทางเลือกที่มีให้เลือก (ต้องให้ผู้ใช้ตัดสิน):**

| ทาง | ต้องทำอะไร | ได้อะไร |
|---|---|---|
| **A1** | ผู้ใช้อัปโหลดเฉพาะโฟลเดอร์ `data/gui/` (~ไม่กี่ MB) เข้ามาใน workspace | Phase 5 ใช้โลโก้/พื้นหลังต้นฉบับได้ถูกกฎ 100% |
| **A2** | ผู้ใช้อัปโหลด `RAN-EP7-ThaiPack-FamHawaii.zip` (1.36 MB — เล็กพอ) | ได้ `.ntk`/`.qst` ไทย + UI text ไปทดสอบ UI |
| **A3** | ผู้ใช้อัปโหลด `.rcc` / subset ของ map แรก (`login.wld`, `s01_01.chf`) | เริ่ม Phase 3.1/3.3 ได้ |
| **A4** | ยอมให้ใช้ **procedural UI ชั่วคราว** (ไม่ใช้ภาพ) จนกว่าจะได้ asset | ทำ Phase 5 ต่อได้ทันที แต่ **ผิดกฎ "ใช้ asset เดิม"** |

> ผมยัง**ไม่ทำ** A4 เพราะ Master Command ระบุชัดว่า *"ถ้ามี asset ภาพพื้นหลัง/โลโก้ต้นฉบับอยู่แล้วใน repo ให้ใช้ของเดิม"* — ต้องรอคำยืนยันว่ายอมรับ procedural ชั่วคราวหรือไม่

---

## B2 — ไม่มี Android SDK / NDK / Gradle / adb ใน environment นี้ (บล็อก Phase 6 local)

| | |
|---|---|
| **สถานะ** | 🟡 `WORKAROUND` (ใช้ GitHub Actions แทนได้) |
| **กระทบ** | การ build APK/AAB และการรันบน device จากเครื่องนี้ |

**หลักฐาน:**
```
g++ ✔  cmake ✘  ninja ✘  java ✘  gradle ✘  adb ✘   ANDROID_HOME=unset  ANDROID_NDK_HOME=unset
```

**Workaround ที่ใช้ได้แล้ว:** workflow `Build RANMobile Android (ARM64)` บน `ubuntu-latest` เคย build สำเร็จ (run `45657604176` → success) ⇒ **Phase 6 ทำผ่าน CI ได้**

**ยังต้องรอผู้ใช้สำหรับ:** การทดสอบบน **อุปกรณ์จริง** (ไม่มี adb/emulator) → ต้องให้ผู้ใช้ติดตั้ง APK แล้วรายงานผลตาม `TESTING_CHECKLIST.md`

---

## B3 — ไม่มี EP7 Server จริง + MSSQL (บล็อกการทดสอบ Phase 3.4 end-to-end และ Phase 4)

| | |
|---|---|
| **สถานะ** | ✅ `RESOLVED` (ผู้ใช้ตัดสินใจ 2026-11-25: **ยอมรับ offline protocol test vector แทน** — ดู `PROTOCOL_FIX_REPORT.md`) |
| **กระทบ** | ยืนยันว่า login ผ่านจริง, ทดสอบ movement/combat, Phase 4 |

**สิ่งที่ขาด:** `LoginServer` + `AgentServer` + `FieldServer` + `SessionServer` ที่รันบน Windows, MSSQL ที่ restore `RanUser`/`RanGame1`/`RanLog`/`RanShop`

**สิ่งที่ต้องการจากผู้ใช้ (เลือก 1):**
- **B3-a**: ให้ endpoint (IP:port) ของ EP7 server ที่ทดสอบได้ + บัญชีทดสอบ 1 บัญชี
- **B3-b**: ยืนยันว่ายอมรับ **offline protocol test** (test vector จาก `minTea` + struct layout) แทน โดยไม่ต้องมี server จริง
- **B3-c**: เปิดทางให้ download `RAN-EP7-ServerDBTools.zip` (49 MB — อยู่ในลิสต์ที่ sandbox โหลดไม่ได้)

> หมายเหตุ: TEA implementation เป็น deterministic — ผมเขียน test vector ที่ตรวจได้โดยไม่ต้องมี server ได้ (ถ้าเลือก B3-b)

---

## B4 — `RanLogic/` ไม่มีใน repo (บล็อก Phase 4 เต็มรูปแบบ)

| | |
|---|---|
| **สถานะ** | ✅ `RESOLVED` (ผู้ใช้ตัดสินใจ 2026-11-25: **ข้ามไปก่อน — Phase 4 ทำแค่ระดับ protocol/auth**) |
| **กระทบ** | Phase 4 "SERVER (GM) COMPATIBILITY" |

**ข้อเท็จจริง:** repo `famoh01/ran-online-ep7` **ไม่มี** โฟลเดอร์ `RanLogic/`, `RanLogicClient/`, `RanLogicServer/`
```bash
ls -d /tmp/ransrc/RanLogic*
# → No such file or directory
```
ที่มีคือ `RanClientLib/G-Logic/` (109 cpp) และ `enginelib/G-Logic/` ซึ่งเป็น **client-side controller** ไม่ใช่ shared game logic สำหรับ server

**ผล:** ปรับ server ให้รองรับ mobile ได้ในระดับ **protocol/auth layer** เท่านั้น (ซึ่งเป็นสิ่งเดียวที่ mobile ต้องใช้จริง) — ถ้าต้องการ gameplay logic ฝั่ง server ต้องมี `RanLogic*`

**ต้องการจากผู้ใช้:** อัปโหลด `RanLogic/` จากชุดซอร์สเต็ม (ใน `RAN-EP7-ServerDBTools.zip` หรือ repo สำรอง) **หรือ** ยืนยันว่าขอบเขต Phase 4 = protocol/auth เท่านั้น

---

## B5 — Release keystore (บล็อก signed release)

| | |
|---|---|
| **สถานะ** | ✅ `RESOLVED` (ผู้ใช้ตัดสินใจ 2026-11-25: **ข้ามไปก่อน — จัดการตอน Phase 6**) |
| **กระทบ** | Release APK/AAB ที่ติดตั้ง/อัปสโตร์ได้ |

ไม่มี `.jks` และไม่มีรหัสผ่านใน repo (ถูกต้องตามหลักความปลอดภัย — **ห้ามสร้างแทน**)
ปัจจุบัน CI ออก `app-debug.apk` (debug-signed) และ `app-release-unsigned.apk`

**ต้องการจากผู้ใช้:** สร้าง secrets `KEYSTORE_BASE64`, `KEYSTORE_PASSWORD`, `KEY_ALIAS`, `KEY_PASSWORD` ใน GitHub repo settings **หรือ** ยืนยันว่าใช้ debug APK สำหรับทดสอบไปก่อน

---

## B6 — Google Drive ไฟล์ใหญ่เกินเพดาน connector

| | |
|---|---|
| **สถานะ** | 🟡 `WORKAROUND` |
| **กระทบ** | ดึง asset จาก Drive ตรงๆ |

| ไฟล์ | ขนาด | ดึงได้ไหม |
|---|---|---|
| `RAN-EP7-ThaiPack-FamHawaii.zip` | 1.36 MB | ✅ ได้ |
| `RAN-EP7-ItemText-Thai-Update.zip` | 0.91 MB | ✅ ได้ |
| `RanOnlineEP7-Server.zip` | 19.2 MB | ✅ ได้ |
| `RanOnlineEP7-Client.zip` | 22.4 MB | ✅ ได้ |
| `RAN-EP7-ServerDBTools.zip` | 49.8 MB | ✅ ได้ (แต่ไม่ใช่ asset) |
| `RAN-EP7-Client-TH.00.zip` | 1.57 GB | ❌ เกิน 100 MB |
| `RAN-EP7-Client-TH.001.zip` | 1.01 GB | ❌ เกิน 100 MB |

**Workaround:** ถ้า `RanOnlineEP7-Client.zip` (22 MB) มี asset ที่ต้องการ ผมดึงได้เลย — **แต่ต้องรอคำยืนยัน** ว่าจะให้ดึง/แตกเข้า workspace หรือไม่ (มันจะเพิ่มขนาด workspace มาก)

---

## B7 — ไม่มี asset ทำให้ unit test ในเครื่องนี้รันไม่ครบ

| | |
|---|---|
| **สถานะ** | 🟡 `WORKAROUND` |
| **กระทบ** | ตรวจ regression ในเครื่องนี้ |

```
$ ./ranmobile_tests
ranmobile_tests: RANMobile/Tests/test_main.cpp:186: int main(): Assertion `tpStats.found' failed.
```
บรรทัด 186 คือ `assert(tpStats.found)` จาก `ThaiPackLoader::InspectThaiPack("external/ep7_release/thaipack_original")` — path นี้มีแค่บน CI

**Workaround:** แยก test ที่ต้อง asset ออกเป็นกลุ่ม `ASSET_TESTS` (opt-in ด้วย `--with-assets`) เพื่อให้ core test (protocol/TEA/parser) รันได้ทุกที่ → จะทำใน Phase 6

---

## สรุป: อะไรทำได้ต่อทันทีโดยไม่ต้องรอ

| งาน | สถานะ |
|---|---|
| Phase 3.4 — แก้ login protocol (**2079** + `THAI_NET_LOGIN_DATA` 54B + TEA) + test vector | ✅ **เสร็จแล้ว** (ดู `PROTOCOL_FIX_REPORT.md` — หมายเหตุ: opcode ที่ถูกคือ 2079 ไม่ใช่ 2142) |
| Phase 3.4 — frame reassembly + heartbeat ตามรอบ | ✅ ทำได้เลย |
| Phase 5 — Login UI responsive + touch feedback + keyboard inset (procedural/vector ตามคำตัดสิน B1) | ✅ ทำได้เลย |
| Phase 6 — แยก test suite + CI + `TESTING_CHECKLIST.md` | ✅ ทำได้เลย |
| Phase 3.1/3.3 — mesh/asset pipeline (procedural/vector ตามคำตัดสิน B1) | ✅ ทำได้เลย (ไม่มี asset ต้นฉบับ — ห้ามกุข้อมูลเกม) |
| Phase 4 — server compat ระดับ protocol/auth (ตามคำตัดสิน B3/B4) | ✅ ทำได้เลย |
| Phase 6 — signed release | ⏸️ รอ **B5** (จัดการตอน Phase 6) |

**Phase 3.4 เสร็จแล้ว — หยุดรอคำสั่งไป Phase ถัดไปตามกฎข้อ 4**


## อัปเดตหลัง Phase 3.4 งาน B — 2026-11-25

- **B2:** ผู้ใช้อนุมัติเลื่อนการทดสอบ device จริงไป Phase 6; งานนี้ตรวจ host/offline เท่านั้น
- **B7:** ผู้ใช้อนุมัติ workaround แยก suite แต่ให้เลื่อนการปรับ suite เดิมไปภายหลัง;
  ไม่แก้ `test_main` หรือ asset tests ในงาน B
- Frame reassembly + heartbeat งาน B เสร็จแล้ว ดู `PROTOCOL_FIX_REPORT.md` §5
- ยังไม่มี live EP7 interoperability/device test; offline tests ไม่ใช่หลักฐานว่า login จริงสำเร็จ
- หยุดรอคำสั่งก่อน Phase 5 (Login UI); ไม่เริ่ม UI ในงานนี้

## B8 — ต้องยืนยัน wire ABI/struct padding ของ PC build จริง

**สถานะ: OPEN — หยุดรอคำตัดสินก่อนอ้างความเข้ากันได้กับ server จริง**

ขณะตรวจ heartbeat งาน B พบว่า `s_NetGlobal.h` ใช้ native structs และส่ง
`sizeof(struct)` ไม่ใช่ผลบวก field อย่างเดียว ไม่พบ pack(1) ใน header/ค่ากำหนด
StructMemberAlignment ใน netclientLib project ที่ตรวจ Default 4-byte alignment ให้:
- heartbeat answer / NET_ENCRYPT_KEY = **24B** (8+13+3 padding) — งาน B ใช้ค่านี้
- THAI_NET_LOGIN_DATA = **56B** (8+4+21+21+2 padding) — ขัดกับ **54B** ที่งาน A
  เคยรายงาน/ทดสอบและผู้ใช้รับทราบ (งาน B ยังไม่เปลี่ยน login struct)

Offline assert ของงาน A ตรวจเพียง mobile packed struct ไม่ได้ยืนยัน ABI ของ PC
จึงไม่ใช่หลักฐานว่า PC ส่ง 54B จริง ขอแก้ความมั่นใจของรายงานเดิมตรงนี้อย่างชัดแจ้ง
opcode **2079** และลำดับ password/userid ไม่ได้รับผลกระทบ

**ต้องการอย่างใดอย่างหนึ่ง:**
1. ผล `sizeof(THAI_NET_LOGIN_DATA)`, `sizeof(NET_HEARTBEAT_CLIENT_ANS)`,
   `sizeof(NET_ENCRYPT_KEY)` จาก PC/server build ที่จะใช้งาน หรือ packet capture
   (ไม่ส่ง credential จริง), หรือ
2. อนุมัติงานแก้ ABI แยกโดยยึด default native PC alignment (login 56B,
   heartbeat/key 24B) พร้อมแก้ tests/รายงานงาน A; ไม่ใช้วิธีรับทั้งสองแบบโดยเดา

การเลื่อน device test B2 และยอมรับ offline B3 ไม่ยืนยัน ABI แทนหลักฐานนี้
งาน B commit เป็นงานสำหรับ review; ยังไม่ยืนยัน end-to-end และยังไม่ไป Phase 5

### B8 — คำตัดสินและผล follow-up (supersede OPEN ด้านบน)

**สถานะ: RESOLVED สำหรับ implementation / verification deferred Phase 6**
ผู้ใช้อนุมัติให้ใช้ native layout หากไม่พบ effective packing override
ตรวจ build flags/global props และ common/PCH/include chain แล้วไม่พบ override
ที่ครอบ struct เป้าหมาย (มี pragma ภายใน SDK/shared บางแห่งแต่ reset/pop หรืออยู่ภายหลัง)
รายละเอียดหลักฐาน/ข้อจำกัดใน `PROTOCOL_FIX_REPORT.md` §6

ใช้ login **56B** / heartbeat **24B**, opcode **2079** ไม่เปลี่ยน
นี่คือ **best evidence จาก static analysis ของซอร์ส ยังไม่ยืนยันด้วย live packet capture**
Phase 6 (B2 device/server test) ต้องยืนยัน sizeof ของ target และ wire capture,
field offsets, heartbeat/key sizes และ rolling TEA ≥2 รอบก่อน claim live compatibility
ไม่ต้องรอ blocker นี้เพื่อปิดงาน static ABI ตามคำอนุมัติ; ยังไม่เริ่ม Phase 5
