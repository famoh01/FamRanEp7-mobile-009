# FamRanEp7-mobile-009 — RAN Online EP7 · PC → Mobile (Android) Port

งานพอร์ต **RAN Online EP7** จาก PC (Win32/MFC/DirectX 9) เป็น **Android** ทำงานต่อจากงานเดิมใน
[`famoh01/ran-online-ep7`](https://github.com/famoh01/ran-online-ep7) (PR #1, #2 — merged)

> ⚠️ **สถานะ: Phase 1–2 เสร็จ** · ยังไม่เริ่ม Phase 3 — รอคำสั่งไป Phase ถัดไป

## เอกสารสถานะ (อ่านตามลำดับ)

| ไฟล์ | เนื้อหา |
|---|---|
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | **Phase 1** — โครงสร้างจริง, client/server แยกตรงไหน, ไลบรารีที่ใช้, ไฟล์ Login/CharSelect/Main UI, ground truth ของ protocol |
| [`PROGRESS_REVIEW.md`](PROGRESS_REVIEW.md) | **Phase 2** — งานเดิมสมบูรณ์แค่ไหน, defect ที่พบ (protocol), แผนลำดับถัดไป |
| [`BLOCKERS.md`](BLOCKERS.md) | สิ่งที่ติดและต้องรอผู้ใช้ตัดสิน (B1–B7) |

## โครงสร้าง repo

```
RANMobile/        ← งาน mobile เดิม (import มาจาก upstream, ไม่ได้เขียนใหม่)
├── Engine/       Core · Renderer (GLES3) · Resource · Touch · Camera · Animation · Audio · Network
├── Client/       Login · Character · Gameplay · Combat · Inventory · Quest · Chat · Map
├── UI/           Login · HUD · Buttons · Joystick · Quest · Map · Chat · Inventory · Skills · Settings
├── Data/         Importer · Converter · Validator · Loader (อ่าน asset EP7 เท่านั้น — ไม่กุข้อมูล)
├── Android/      AndroidManifest.xml · native_bridge.cpp · build.gradle
├── Server/       MobileServerHost (host ทดสอบ protocol)
└── Tests/        test_main.cpp
```

ยัง**ไม่ได้** import ซอร์ส PC/Server (~8,900 ไฟล์, 200+ MB) เพราะ Phase 1–2 ใช้เป็นข้อมูลอ้างอิงเท่านั้น
(ซอร์สอ้างอิงดูได้ที่ upstream ตาม path ที่ระบุใน `ARCHITECTURE.md`)

## กฎที่ใช้ตลอดโครงการ

1. ไม่สร้าง/แต่ง Monster, NPC, Item, Skill, Quest, Map ที่ไม่มีในซอร์สต้นฉบับ
2. Protocol / packet / game logic ต้องตรงต้นฉบับ 100% — แก้ได้เฉพาะ layer แสดงผล/input/transport/UI
3. แยก PR ตาม Phase
4. Blocker → บันทึกใน `BLOCKERS.md` แล้วหยุดรอคำสั่ง
5. ไม่ merge เข้า `main` เอง — เปิดเป็น Draft PR เสมอ

## Build (ต้องมี Android NDK r25+ / SDK 34 / CMake 3.22+)

ดู [`RANMobile/BUILD_ANDROID.md`](RANMobile/BUILD_ANDROID.md)
ใน environment ที่ไม่มี NDK ยัง compile ฝั่ง Linux เพื่อรัน test ได้:

```bash
g++ -std=c++17 -Wall -Wextra -Werror $(find RANMobile -name '*.cpp' ! -name 'native_bridge.cpp' ! -name 'dep_scan_cli.cpp' ! -name 'test_protocol.cpp' ! -name 'test_stream.cpp') \
    -lpthread -o ranmobile_tests
```
