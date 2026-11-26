# BLOCKERS.md — RAN Online EP7 · PC → Mobile

> อัปเดต: 2026-11-25 (Phase 1, branch `arena/7d98df66-famranep7-mobile-009`)
> กฎ: เจอ blocker = บันทึกที่นี่ + ระบุสิ่งที่ต้องการ + หยุดรอคำสั่งถัดไปก่อนข้าม (Master Command ข้อ 4)

## สรุปสถานะ

| ID | Blocker | สถานะ | กระทบ Phase | ต้องการจากผู้ใช้ |
|---|---|---|---|---|
| B1 | Asset เกมต้นฉบับไม่อยู่ใน Git / ไฟล์ใหญ่เข้าถึงยาก | OPEN (มี workaround บางส่วน) | 3.1,3.3,5 | ตัดสินใจแนวทาง asset (ดูด้านล่าง) |
| B2 | ไม่มี Android SDK/NDK/Gradle/adb ใน sandbox | WORKAROUND (CI) | 6, device test | ทดสอบบน device จริงโดยผู้ใช้ หรือยอมรับ CI-only |
| B3 | ไม่มี Live EP7 Server + MSSQL | OPEN (offline verify ได้บางส่วน) | 3.4 E2E,4,6 | ให้ endpoint+บัญชีทดสอบ หรือยอมรับ offline protocol test |
| B4 | `RanLogic/`, `RanLogicClient/`, `RanLogicServer/` ไม่มีใน upstream repo | OPEN | 4 เต็มรูปแบบ | อัปโหลด/ชี้แหล่ง RanLogic* หรือจำกัด Phase 4 เป็น protocol/auth |
| B5 | Release keystore ไม่มี | OPEN (debug ได้) | 6 release | ให้ secrets keystore หรือยอมรับ debug/unsigned |
| B6 | Google Drive ไฟล์ใหญ่ / release asset host จำกัด | WORKAROUND | 1,3.1,3.3 | อัปโหลดไฟล์เฉพาะที่จำเป็นเข้าสู่ workspace/repo ที่เข้าถึงได้ |
| B7 | Test บางส่วนพึ่ง asset ภายนอก | WORKAROUND | 6 | อนุมัติแยก asset-dependent tests เป็น opt-in |

## รายละเอียด

### B1 — Asset เกมต้นฉบับ
- **ข้อเท็จจริง**: upstream git ไม่มี `.rcc/.dds/.ntk/.wld/.chf/.cps` และไม่มีโฟลเดอร์ gui/skin/texture ตาม PR #2 audit. Release `ran-ep7-full-v1` มี client split zip ใหญ่ (1.57GB+1.01GB) แต่ release-assets host ถูกจำกัดใน sandbox ก่อนหน้า; Google Drive ไฟล์ใหญ่เกินเพดานที่เหมาะสม.
- **ผลกระทบ**: Phase 3.1/3.3 ไม่สามารถ validate กับ asset จริงครบได้; Phase 5 ต้องใช้โลโก้/พื้นหลังต้นฉบับถ้ามี — แต่ asset ไม่อยู่ใน Git.
- **ทางเลือก**:
  - A: ผู้ใช้อัปโหลดเฉพาะ `data/gui` หรือไฟล์ที่ Phase 5 ต้องใช้ (เล็ก) เข้ามา workspace
  - B: อนุญาตให้ใช้ **procedural/vector UI ชั่วคราว** สำหรับ Phase 5 (ไม่แต่งข้อมูลเกม) และสลับเป็น asset เดิมเมื่อมีไฟล์ — ต้องการคำยืนยันชัดเจน
  - C: ให้สิทธิ์/วิธีดาวน์โหลดที่ sandbox เข้าถึงได้
- **สถานะ**: รอคำตัดสินผู้ใช้

### B2 — ไม่มี Android SDK/NDK ใน sandbox
- **ตรวจพบ**: `cmake`, `ninja`, `java`, `gradle`, `adb` ไม่พบในเครื่องนี้ (อ้างอิง PR #2 และสภาพ sandbox ปัจจุบัน).
- **Workaround**: ใช้ GitHub Actions (เคยเขียวใน upstream). Local build ทำไม่ได้
- **ต้องการ**: ถ้าต้องทดสอบ device จริง ผู้ใช้ต้องติดตั้ง APK และรายงานผล

### B3 — ไม่มี Live EP7 Server + MSSQL
- **ผลกระทบ**: Thai login/protocol verify ได้แค่ offline/unit-test/vector; ไม่สามารถยืนยัน live interoperability
- **ทางเลือก**: ให้ test endpoint+credentials ผ่านช่องทางปลอดภัยที่เหมาะสม, หรือยอมรับ offline verification + capture checklist สำหรับ Phase 6

### B4 — RanLogic* ไม่มีใน upstream
- **ข้อเท็จจริง**: `RanLogic/`, `RanLogicClient/`, `RanLogicServer/` ไม่มีใน `famoh01/ran-online-ep7` ตาม PR #2 audit.
- **ผลกระทบ**: Phase 4 ทำได้เต็มที่เฉพาะ protocol/auth/session layer ที่จำเป็นต่อ mobile; gameplay logic ฝั่ง server ต้องมี source เพิ่ม
- **ต้องการ**: ผู้ใช้ระบุขอบเขต Phase 4 ว่า protocol/auth เพียงพอหรือไม่ หรือจัดหา RanLogic source

### B5 — Release keystore
- ไม่มี `.jks`/credentials ใน repo; ไม่สร้างแทน. CI ออก debug/unsigned ได้

### B6 — แหล่งไฟล์ใหญ่
- `RAN-EP7-Client-TH.zip.part00/01` ใหญ่เกินกว่าจะดึงแบบสะดวก; release-assets host มีข้อจำกัด. Repo สำรองมี source+compiled tools แต่ไม่ใช่ client asset เต็มชุดเดียวกัน
- **ต้องการ**: ผู้ใช้ชี้ไฟล์เฉพาะที่ต้องใช้และวิธีส่งที่เข้าถึงได้

### B7 — Asset-dependent tests
- `RANMobile/Tests/test_main.cpp` ใน PR #2 เคย abort เมื่อไม่มี `external/ep7_release/thaipack_original`.
- **Workaround**: แยก suite เป็น core vs asset-opt-in ใน Phase 6

## สิ่งที่ทำได้ต่อโดยไม่ติด blocker (หลังมี code baseline)

- นำ RANMobile baseline จาก PR #2 มาวางบน branch ปัจจุบันอย่างเป็นระเบียบ (docs/code) เพื่อทำงานต่อ
- ตรวจ/รักษา protocol correctness (2079/TEA/56B/reassembly/heartbeat) ด้วย offline tests
- ออกแบบ Login UI ใหม่แบบ procedural/vector (ถ้าผู้ใช้ยืนยัน B1 ทางเลือก B)
- เตรียม CI/workflow และ testing checklist

**หยุดรอคำสั่งผู้ใช้หลังรายงาน Phase 1** — โดยเฉพาะ B1 (asset/procedural UI) และทิศทางการนำ baseline PR #2 เข้ามาใน branch ปัจจุบัน
