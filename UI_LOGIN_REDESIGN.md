# Phase 5 — Login UI survey & empty scaffold handoff

วันที่: 2026-11-25 · ขอบเขตที่อนุมัติ: สำรวจ + เตรียม stub เท่านั้น
**ไม่มีดีไซน์ใหม่ สี ขนาด layout, animation หรือ glass-effect implementation ในงานนี้**
Protocol Phase 3.4 คงเดิมจนถึง Phase 6 (opcode 2079 / login 56B / heartbeat 24B)

## 1. Entry points และ lifecycle (เลขบรรทัดหลังเปลี่ยนเป็น stub)

| จุด | ไฟล์:บรรทัด | หน้าที่ |
|---|---|---|
| Android entry | `RANMobile/Android/native_bridge.cpp:201` | `android_main`, ลงทะเบียน callbacks และ event/render loop |
| EGL init | `RANMobile/Android/native_bridge.cpp:22` | `InitEGL`, สร้าง context/window surface และเรียก Initialize/Resize/Start |
| สร้างหน้า Login | `RANMobile/Engine/Core/RANMobileEngine.cpp:32` | `make_unique<LoginScreen>()` |
| เชื่อม NetworkClient | `RANMobile/Engine/Core/RANMobileEngine.cpp:40` | `LoginScreen::Initialize(m_networkClient.get())` |
| หน้า Login class | `RANMobile/UI/Login/LoginScreen.h:30` | global `LoginScreen` (คง namespace/API เดิมเพื่อไม่กระทบ caller) |
| Login init stub | `RANMobile/UI/Login/LoginScreen.cpp:4` | เก็บ non-owning network pointer + server metadata เดิม |
| Update | `RANMobile/Engine/Core/RANMobileEngine.cpp:65` | Update Login ที่ :76 และ network Poll ที่ :79 |
| Render dispatch | `RANMobile/Engine/Core/RANMobileEngine.cpp:83` | BeginFrame → ถ้า state Login เรียก Draw(renderer) ที่ :89 → EndFrame |
| Android frame loop | `RANMobile/Android/native_bridge.cpp:223-225` | Update → Render → eglSwapBuffers |
| Touch routing | `RANMobile/Engine/Core/RANMobileEngine.cpp:112-124` | OnTouchDown/Move/Up → Login handlers |
| Text routing | `RANMobile/Engine/Core/RANMobileEngine.cpp:127-132` | OnTextInput / OnBackspace → Login handlers |
| Platform input | `RANMobile/Android/native_bridge.cpp:139` | `HandleInputEvent`, motion/key events; Enter เรียก OnLoginPressed |
| Submit seam | `RANMobile/UI/Login/LoginScreen.cpp:20-21` | เรียก AuthenticationManager::Login โดยตรง ไม่สร้าง packet ใน UI |
| Draw hooks | `RANMobile/UI/Login/LoginScreen.cpp:25-26` | ทั้งสอง overload เป็น no-op รอผู้ใช้เขียน |

Default screen state คือ Login (`RANMobileEngine.h`); ปัจจุบัน engine ส่ง touch ให้ Login
โดยไม่ guard screen state ที่จุด input ขณะที่ Render มี guard แยก งานนี้ไม่เปลี่ยน routing

## 2. Rendering capability ที่มีจริง

### Backend / shader

- Android GL implementation คือ **OpenGL ES 3.0** (`MobileRenderer.cpp:10-12`)
  ลิงก์ EGL/GLESv3 ใน CMake ไม่มี Vulkan backend implementation
- Android bridge พยายาม fallback ไป ES2 config/context แต่ renderer ใช้
  `#version 300 es`, VAO และ GL_R8/GL_RED ของ ES3 จึง **ไม่ใช่ ES2 fallback
  ที่ใช้งานได้ครบ** อย่าเขียน UI โดยสมมติว่ารองรับ ES2 แล้ว
- บน host/non-Android เป็น **headless path** ไม่มี window/GL renderer จริง
  การ compile/tests บน host ไม่ยืนยัน shader/device behavior
- UI shader อยู่ใน `MobileRenderer.cpp:23-51`: GLSL ES 3.00 vertex + fragment
  เป็น embedded strings ไม่ใช่ shader-file loader หรือ material system
- Vertex attributes: vec2 aPos / vec2 aUV, orthographic uProj; ต้นกำเนิด UI มุมซ้ายบน
  และพิกัดเป็น surface pixels ไม่ใช่ dp (DrawUIQuad :285 และ DrawUIText :319)
- Fragment shader มี uColor, uUseTexture, sampler2D uTex:
  mode 0 = flat color, mode 1 = font alpha จาก red channel, mode 2 = sampled RGBA * color
- Public drawing API ตอนนี้มี **DrawUIQuad / DrawUIText / MeasureTextWidth**
  มี UploadRGBA8Texture/DeleteTexture แต่ **ไม่มี public DrawUIImage/DrawTexturedQuad**
  ที่เลือก mode 2 ให้เรียกใช้ทันที แม้ branch นั้นอยู่ใน shader
- shader handles/CompileShader เป็น private/file-local; ไม่มี public custom shader hook
  ถ้าผู้ใช้จะเพิ่ม effect ต้องแก้/ขยาย renderer เองภายหลัง ไม่ใช่เติม UITheme อย่างเดียว
- Shader compile มีการเช็ก status แต่ link status ไม่ได้ตรวจอย่างครบถ้วน;
  Initialize ยังคืน success จึงต้องตรวจด้วย device ใน Phase 6 ไม่ถือ log ว่าพิสูจน์สำเร็จ

### Transparency / glass

- **Alpha blending มีจริง**: `MobileRenderer.cpp:143-144`
  `glEnable(GL_BLEND)` + `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`
- DrawUIQuad รับ RGBA; shader ส่ง alpha ผ่าน จึงทำแผงสีโปร่งใสทับสิ่งที่วาดก่อนหน้าได้
  (straight alpha ไม่ใช่ premultiplied alpha)
- **ไม่มี frosted/backdrop blur/refraction/glass pipeline**: ไม่มี scene color capture,
  offscreen framebuffer, blur passes หรือ rounded-corner/SDF clipping API ที่ต่อไว้
  โปร่งใสได้ ≠ มี glass blur แล้ว ไม่ควรอ้างว่ารองรับ effect เหล่านี้สำเร็จ
- GLES3 เป็นพื้นฐานที่สามารถเขียน effects เพิ่มได้ แต่ยังไม่ได้ implement หรือทดสอบ
  งานนี้ไม่เพิ่ม shader/renderer effects และไม่แตะ clear color เดิมของ renderer

### Fonts

- **มี font rasterization แล้ว**: bundled `stb_truetype.h`, InitFont + per-codepoint bitmap
  (`MobileRenderer.cpp:108-140,221-282`) และ atlas **1024×1024 GL_R8**, base font 28px
- พยายามโหลด primary จาก `/system/fonts/Roboto-Regular.ttf`, NotoSans, DroidSans;
  ไทยจาก NotoSansThai/NotoSansThaiUI/DroidSansFallback ตามลำดับ ไม่มี font asset
  bundle ที่รับประกันบนทุก device; ถ้าไม่พบ font ให้ advance แต่ไม่วาด glyph
- DrawUIText decode UTF-8 แล้ววาด codepoint ต่อ codepoint; มี Thai-font fallback
  แต่ **ไม่มี shaping engine (HarfBuzz), Thai mark positioning/layout, kerning calls,
  bidi, multiline wrapping หรือ text selection/caret** ใน draw path
  ดังนั้นมี glyph ไทย ≠ ยืนยันการวางสระ/วรรณยุกต์ถูกต้องครบ
- ไม่มี SDF text pipeline แม้ library จะมีฟังก์ชัน SDF; caller ปัจจุบันใช้ bitmap atlas

### Input / keyboard ข้อจำกัดเดิมที่ควรรู้ก่อนเติม UI

- Android keyboard show/hide มี JNI helper (`native_bridge.cpp:102`)
  อาศัย GetFocus หลัง touch; ขณะเป็น stub focus ไม่เปลี่ยนจึงยังไม่เปิด field/keyboard
- key mapping ตอนนี้จำกัด a-z, 0-9 และ punctuation บางตัว ไม่ใช่ Android IME
  commit/composition pipeline สำหรับภาษาไทย/Unicode/shift อย่างครบถ้วน
- ไม่มี keyboard-inset/safe-area API ที่เชื่อม Login อยู่แล้ว และไม่มี responsive
  layout/48dp enforcement ใน stub ผู้ใช้เติมภายหลังตามคำสั่ง

## 3. Stub ที่ส่งมอบ

| ไฟล์ | สถานะ |
|---|---|
| `RANMobile/UI/UITheme.h` | ใหม่: empty `RANMobile::UITheme` ไม่มีสี/ขนาด/default design |
| `RANMobile/UI/Login/LoginScreen.h` | คง API เดิมเป็น compatibility shell เพื่อให้ engine/Android/legacy tests compile |
| `RANMobile/UI/Login/LoginScreen.cpp` | แทน implementation UI เดิมด้วย no-op input/update/draw; เหลือ init และ auth delegation เท่านั้น |
| `RANMobile/UI/Components/UIButton.h/.cpp` | ใหม่: empty reusable class + default ctor/dtor ไม่มี hit testing/state/effect |
| `RANMobile/UI/Components/UIInputField.h/.cpp` | ใหม่: empty reusable class + default ctor/dtor ไม่มี editing/render logic |

**LoginScreen มีอยู่ก่อนแล้ว** จึงไม่สร้าง class ซ้ำที่ path ใหม่ โค้ด layout/สี/input
เดิมถูกถอดออกจาก .cpp ตามคำขอให้เหลือ scaffold ให้ผู้ใช้เขียนทับ; ดูของเดิมได้ใน
commit `06e1b48` API/getters/metadata เดิมใน header คงไว้เพื่อ source compatibility
ไม่ใช่คำยืนยันว่า focus/status/settings/exit/remember-account ทำงานใน stub

OnLoginPressed เพียง delegate ไป AuthenticationManager เดิม:
username/password จาก setters → manager ตรวจข้อมูล/connection → TEA field encryption
→ opcode2079 / 56B ผ่าน NetworkClient ไม่มี logic packet ใหม่
true = sent/queued **ไม่ใช่ login accepted** ยังไม่มี callback login-result UI เชื่อมใน stub
ไม่มี auto-connect/endpoint ใหม่ และไม่มี network callback ที่ถูกแย่งจากผู้ใช้

## 4. Build verification

- เพิ่ม headers และ .cpp ของ components ใน `RANMobile/CMakeLists.txt:34-40`
  source list เดียวใช้ทั้ง host static library และ Android shared library
- ติดตั้ง host CMake ชั่วคราวผ่าน PyPI ลง `/tmp/ranmobile-cmake` (ไม่เพิ่ม dependency
  หรือ binary ใน repo); ไม่ได้ติดตั้ง Android SDK/NDK หรือเปลี่ยน protocol
- **รัน CMake configure + build ทุก host target จริง ผ่าน** ด้วย
  `-DCMAKE_CXX_FLAGS='-Wall -Wextra -Werror'`
- build log ยืนยัน compile UIButton.cpp, UIInputField.cpp, LoginScreen.cpp และ link
  ranmobile_core / ranmobile_tests / ranmobile_protocol_tests / ranmobile_stream_tests
- temporary smoke harness instantiate ทั้งสาม classes + UITheme, เรียก no-op hooks
  และทดสอบ OnLoginPressed เมื่อ null connection: manager ปฏิเสธตามเดิม **ผ่าน**
- รัน protocol และ stream suites จาก CMake build **ผ่าน** (ไม่มีการแก้ protocol sources)
- **ไม่รัน test_main suite เต็ม** ตาม B7; legacy tests ที่ assert layout/focus/settings
  ของหน้าเดิมจะต้องปรับเมื่อผู้ใช้ใส่ UI จริง ไม่เปลี่ยน tests เพื่ออ้างว่า stub มี features เดิม
- Android/device/GPU execution ยังไม่ได้ตรวจตาม B2 → Phase 6

คำสั่ง standard เมื่อติดตั้ง CMake แล้ว:
```sh
cmake -S RANMobile -B /tmp/ranmobile-phase5-build \
  -DCMAKE_CXX_FLAGS='-Wall -Wextra -Werror'
cmake --build /tmp/ranmobile-phase5-build -j2
/tmp/ranmobile-phase5-build/ranmobile_protocol_tests
/tmp/ranmobile-phase5-build/ranmobile_stream_tests
```

## 5. Handoff / stop

งานสำรวจ + stubs ครบแล้ว ไม่มี blocker ใหม่ที่ต้องตัดสินเพื่อส่งมอบ scaffold
ขณะนี้ Login ไม่วาด form และ input handlers ไม่ทำงานโดยตั้งใจ จนกว่าผู้ใช้จะเติม
ไม่มีการเพิ่มดีไซน์ สี ขนาด animation ภาพ หรือเกมคอนเทนต์
Protocol/Auth/Network/Renderer/Android bridge ไม่แก้ในงานนี้
หยุดรอผู้ใช้เขียน/ส่ง UI code ก่อนดำเนินงานถัดไป
