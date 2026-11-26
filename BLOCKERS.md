# BLOCKERS.md — RAN Online EP7 · PC → Mobile

> อัปเดต: 2026-11-25 (หลัง Consolidate Baseline + Phase 2)
> Branch: `arena/7d98df66-famranep7-mobile-009`
> กฎ: เจอ blocker = บันทึกที่นี่ + ระบุสิ่งที่ต้องการ + หยุดรอคำสั่งถัดไปก่อนข้าม (Master Command ข้อ 4)

## สรุปสถานะ

| ID | Blocker | สถานะ | กระทบ | การตัดสินใจ/หมายเหตุ |
|---|---|---|---|---|
| B1 | Asset เกมต้นฉบับ (logo/bg/UI bitmaps) ไม่อยู่ใน Git | **RESOLVED (policy)** | 3.1,3.3,5 | ผู้ใช้อนุมัติ **ทางเลือก B** (2026-11-25): ใช้ Procedural/Vector UI ชั่วคราวสำหรับ Phase 5; ห้ามใช้ asset เกมที่ไม่มีสิทธิ์/ไม่มีไฟล์; เตรียม architecture ให้สลับ asset จริงภายหลัง; โลโก้เกมใช้ของจริงได้เฉพาะเมื่อมีไฟล์ที่เข้าถึงได้ถูกสิทธิ์ |
| B2 | ไม่มี Android SDK/NDK/Gradle/adb ใน sandbox | **RESOLVED (scope)** | 6 local build | ผู้ใช้อนุมัติ B2 (2026-11-25): ยอมรับ CI-only build ใน sandbox; ผู้ใช้ทดสอบบนอุปกรณ์จริงเอง |
| B3 | ไม่มี Live EP7 Server + MSSQL | **RESOLVED (scope)** | 3.4 E2E,4,6 | ผู้ใช้อนุมัติ B3 (2026-11-25): ยอมรับ offline protocol verification + capture checklist ไปก่อน; ยังไม่มี live server/บัญชีทดสอบ |
| B4 | `RanLogic*` ไม่มีใน upstream repo | **RESOLVED (scope)** | 4 | ผู้ใช้อนุมัติ B4 (2026-11-25): จำกัด Phase 4 เฉพาะ protocol/auth compatibility layer ที่ mobile client ต้องใช้; ไม่ทำ full RanLogic porting |
| B5 | Release keystore ไม่มี | OPEN (deferred) | 6 release | ยังไม่ใช่งานรอบนี้; debug/unsigned สำหรับ CI ได้; ต้องให้ secrets เมื่อจะทำ release signing |
| B6 | ดึงไฟล์เสริมจาก Release/Drive | **PARTIAL** | 1,3.3,4 | ผู้ใช้อนุมัติ B6 (2026-11-25): ThaiPack 1.36MB **นำเข้าแล้ว** (fallback ผ่าน Google Drive เนื่องจาก release-assets host ถูกบล็อก); ServerDBTools 49.8MB **ยังนำเข้าไม่ได้** (ดู B6a); Client-TH part ใหญ่ไม่ดึงในรอบนี้ตามคำสั่ง |
| B6a | `RAN-EP7-ServerDBTools.zip` ดาวน์โหลดไม่ได้ใน sandbox | **OPEN** | 4 schema reference | พยายามแล้ว 3 ทาง: `gh release download` → redirect ไป `release-assets.githubusercontent.com` (EOF/blocked); `gh api .../releases/assets/... -H Accept:application/octet-stream` → redirect เดียวกัน (EOF); `google_drive download_file` (file id `1YZdc1OqAe8rpEfBZ6j_kb9YarLZSZtrO`) → "Could not retrieve the file..." ต้องการ: ผู้ใช้อัปโหลดไฟล์เข้า workspace, หรือวางใน path ที่ sandbox อ่านได้, หรืออนุญาตให้ใช้ schema ที่มีอยู่ใน repo สำรอง/เอกสารอื่นแทน |
| B7 | Test บางส่วนพึ่ง asset ภายนอก | **RESOLVED** | 6 | ผู้ใช้อนุมัติแยก (2026-11-25). `test_main.cpp` guard ThaiPackLoader ด้วย env `RANMOBILE_TEST_ASSETS=1`; default run จะ skip asset checks. หมายเหตุเดิมเรื่อง LoginScreen stub mismatch ถูกแก้ใน Phase 5 (Step 1) แล้ว; `test_main` ผ่านใน sandbox โดยไม่ต้องใช้ asset จริง. |
| B8 | ไม่มีไฟล์ Thai font (.ttf/.otf) ที่มีสิทธิ์ bundle ใน repo | **OPEN** | 5 (Thai text rendering) | ThaiPack ไม่มี .ttf/.otf. `MobileRenderer` พยายามโหลด system fonts บน Android (`/system/fonts/NotoSansThai-*.ttf`) แต่ sandbox ไม่มี font และ repo ไม่มี bundled font. ผล: DecodeUTF8/MeasureTextWidth ทำงาน แต่ glyph rendering จริงบนเครื่องที่ไม่มี Thai font จะ fallback/ว่าง. ต้องการจากผู้ใช้: (A) อัปโหลดไฟล์ font ไทยที่มีสิทธิ์ใช้งาน (เช่น NotoSansThai-Regular.ttf, SIL/OFL) เข้า repo/assets, หรือ (B) อนุมัติให้ bundle NotoSansThai จาก Google Fonts (SIL OFL) ในรอบถัดไป. ชั่วคราว: ใช้ system font fallback บน Android; ไม่ block Phase 5 logic/layout tests. |

## รายละเอียดการปิด Blocker

### B1 — Procedural/Vector UI ชั่วคราว (อนุมัติ 2026-11-25)
- อนุมัติทางเลือก B. ข้อกำหนดที่ต้องปฏิบัติใน Phase 5:
  - ห้ามใช้ asset เกมจริงที่ไม่มีสิทธิ์/ไม่มีไฟล์
  - UI chrome (พื้นหลัง, panel, border, glow) ต้อง generate จากโค้ด (shader/vector/gradient) ไม่ใช้บิตแมปจากเกมต้นฉบับ
  - โลโก้เกมต้นฉบับใช้ของจริงได้เฉพาะเมื่อมีไฟล์ที่เข้าถึงได้ถูกสิทธิ์
  - ต้องเตรียม architecture ให้สลับ asset จริงภายหลัง (config/resource key แทน hardcode path)

### B2 — CI-only build (อนุมัติ 2026-11-25)
- Sandbox ไม่มี `cmake`, `ninja`, `java`, `gradle`, `adb` (ตรวจแล้ว). ยอมรับ CI-only build; device test โดยผู้ใช้.

### B3 — Offline protocol verification (อนุมัติ 2026-11-25)
- ไม่มี live server/บัญชีทดสอบในรอบนี้. ใช้ offline test vectors + capture checklist แทน; ห้าม claim live compatibility.

### B4 — Phase 4 scope จำกัด (อนุมัติ 2026-11-25)
- จำกัด Phase 4 เฉพาะ protocol/auth compatibility layer ที่ mobile client ต้องใช้. ไม่ทำ full RanLogic porting.

### B6 — ไฟล์เสริม (อนุมัติ 2026-11-25, partial)
- **ThaiPack** `RAN-EP7-ThaiPack-FamHawaii.zip` (1.36MB): นำเข้าแล้วที่ `RANMobile/Data/External/RAN-EP7-ThaiPack-FamHawaii.zip` และแตกที่ `RANMobile/Data/External/ThaiPack/` (1046 files). แหล่งที่ใช้จริง: Google Drive file `1IL8jlmvl5PGGHdaf5nlnoZdBMhTu25Ag` เพราะ release-assets host ถูกบล็อก.
- **ServerDBTools** 49.8MB: ยังนำเข้าไม่ได้ (B6a).
- **Client-TH big parts**: ไม่ดึงตามคำสั่ง.

## Blocker ใหม่ที่พบระหว่างรอบนี้

### B6a — ServerDBTools download blocked by sandbox/network path
- **สถานะ:** OPEN
- **สิ่งที่พยายาม:**
  1. `gh release download ... --pattern RAN-EP7-ServerDBTools.zip` → EOF จาก `release-assets.githubusercontent.com`
  2. `gh api repos/.../releases/assets/770908216 -H "Accept: application/octet-stream"` → redirect ไป `release-assets.githubusercontent.com` แล้ว EOF
  3. `google_drive.download_file(file_id=1YZdc1OqAe8rpEfBZ6j_kb9YarLZSZtrO)` → "Could not retrieve the file from the connector"
- **ผลกระทบ:** Phase 4 schema reference ยังไม่มีไฟล์จริงใน workspace (ไม่บล็อก Phase 2/3.4 offline)
- **ต้องการจากผู้ใช้:** อัปโหลด `RAN-EP7-ServerDBTools.zip` เข้า workspace หรือให้ path ที่เข้าถึงได้; หรือยืนยันให้ใช้ schema/reference จาก repo สำรอง `matheomedrana/Ran-Online-EP7-Complete-Files` ที่มี server cfg/binaries source tree แทนชั่วคราว

## Conflict/import log

- ระหว่าง import baseline PR #2: พบ add/add conflict ที่คาดได้ 3 ไฟล์ (`ARCHITECTURE.md`, `BLOCKERS.md`, `README.md`).
- วิธีจัดการแบบโปร่งใส: commit Phase 1 docs ก่อน, แล้ว `git checkout FETCH_HEAD -- .` และ restore `ARCHITECTURE.md`/`BLOCKERS.md` จาก HEAD ของ branch นี้; `README.md` ใช้จาก PR #2 baseline. บันทึกไว้ใน commit message `c765f53`. ไม่มีการเลือกข้างเงียบๆ สำหรับเนื้อหา protocol/code; code ทั้งหมดมาจาก PR #2 ตรงๆ.

### B8 — Thai font asset missing (พบระหว่าง Phase 5)
- **สถานะ:** OPEN (non-blocking for Phase 5 logic/layout; blocks guaranteed Thai glyph rendering)
- **สิ่งที่ตรวจพบ:**
  - `RANMobile/Data/External/ThaiPack/` ไม่มี `.ttf/.otf`
  - `MobileRenderer::Initialize` พยายามโหลด system fonts: `/system/fonts/NotoSansThai-Regular.ttf`, `/system/fonts/NotoSansThaiUI-Regular.ttf`, `/system/fonts/DroidSansFallback.ttf`
  - ใน sandbox ไม่มีไฟล์เหล่านี้; บน Android ส่วนใหญ่มี NotoSansThai แต่ไม่รับประกันทุกอุปกรณ์/ROM
- **ผลกระทบ:** Thai text อาจ render ไม่ครบถ้าระบบไม่มี font; unit tests ที่ตรวจ DecodeUTF8/MeasureTextWidth/layout/state ยังผ่าน
- **ข้อเสนอแก้:**
  1. ผู้ใช้อัปโหลด Thai font ที่มีสิทธิ์ (แนะนำ Noto Sans Thai, SIL OFL) เข้า `RANMobile/Assets/Fonts/` แล้วให้ผม wire bundle/load path
  2. หรืออนุมัติให้ดาวน์โหลด NotoSansThai-Regular.ttf (SIL OFL, Google Fonts) ในรอบถัดไป - ต้องยืนยันสิทธิ์/แหล่งที่ผู้ใช้ยอมรับ
  3. ชั่วคราว: พึ่ง system font บน Android + keep layout logic font-metric tolerant
