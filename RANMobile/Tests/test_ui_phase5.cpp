// Phase 5 unit tests: layout calculation, button state transitions, design tokens.
// GLES/runtime rendering not required (B2). No game content/protocol changes.

#include "../UI/Login/LoginScreen.h"
#include "../UI/Components/UIButton.h"
#include "../UI/Components/UIInputField.h"
#include "../UI/UITheme.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

static bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

static void testColorTokens() {
    using namespace RANMobile;
    const UITheme& t = GetDefaultUITheme();
    // Background
    assert(Near(t.bgTop.r, 0x0B / 255.0f) && Near(t.bgTop.g, 0x0E / 255.0f) && Near(t.bgTop.b, 0x14 / 255.0f));
    assert(Near(t.bgBottom.r, 0x1A / 255.0f) && Near(t.bgBottom.g, 0x1F / 255.0f) && Near(t.bgBottom.b, 0x2E / 255.0f));
    // Gold
    assert(Near(t.goldStart.r, 0xD4 / 255.0f) && Near(t.goldStart.g, 0xAF / 255.0f) && Near(t.goldStart.b, 0x37 / 255.0f));
    assert(Near(t.goldEnd.r, 0xF2 / 255.0f) && Near(t.goldEnd.g, 0xD8 / 255.0f) && Near(t.goldEnd.b, 0x8F / 255.0f));
    // Blue
    assert(Near(t.blueStart.r, 0x3A / 255.0f) && Near(t.blueStart.g, 0x5F / 255.0f) && Near(t.blueStart.b, 0xCD / 255.0f));
    assert(Near(t.blueEnd.r, 0x1E / 255.0f) && Near(t.blueEnd.g, 0x3A / 255.0f) && Near(t.blueEnd.b, 0x8A / 255.0f));
    // Glass panel
    assert(Near(t.glassFill.r, 1.0f) && Near(t.glassFill.g, 1.0f) && Near(t.glassFill.b, 1.0f) && Near(t.glassFill.a, 0.06f));
    assert(Near(t.glassBlurPx, 12.0f));
    // Border glow
    assert(Near(t.borderGlow.r, 0xFF / 255.0f) && Near(t.borderGlow.g, 0xD7 / 255.0f) && Near(t.borderGlow.b, 0x00 / 255.0f));
    assert(Near(t.borderGlow.a, 0.40f));
    assert(Near(t.borderWidthPx, 1.5f));
    assert(Near(t.borderGlowMinPx, 4.0f) && Near(t.borderGlowMaxPx, 8.0f));
    // Text
    assert(Near(t.textPrimary.r, 0xF5 / 255.0f) && Near(t.textPrimary.g, 0xF5 / 255.0f) && Near(t.textPrimary.b, 0xF0 / 255.0f));
    assert(Near(t.textSecondary.r, 0x8A / 255.0f) && Near(t.textSecondary.g, 0x8D / 255.0f) && Near(t.textSecondary.b, 0x98 / 255.0f));
    // Status
    assert(Near(t.errorColor.r, 0xFF / 255.0f) && Near(t.errorColor.g, 0x5C / 255.0f) && Near(t.errorColor.b, 0x5C / 255.0f));
    assert(Near(t.successColor.r, 0x4A / 255.0f) && Near(t.successColor.g, 0xDE / 255.0f) && Near(t.successColor.b, 0x80 / 255.0f));
    // Typography
    assert(Near(t.titleSp, 28.0f));
    assert(Near(t.bodySp, 16.0f));
    assert(Near(t.captionSp, 12.0f));
    // Touch/metrics
    assert(Near(t.minTouchTargetDp, 48.0f));
    assert(Near(t.loginButtonHeightDp, 56.0f));
    assert(t.cornerRadiusDp >= 12.0f && t.cornerRadiusDp <= 16.0f);
    std::cout << "[PASS] color/design tokens\n";
}

static void testButtonStateTransitions() {
    using namespace RANMobile;
    UIButton btn;
    btn.SetRect(10, 10, 200, 56);
    btn.SetLabel("TEST");
    btn.SetEnabled(true);
    assert(btn.GetState() == UIButtonState::Default);
    assert(btn.IsEnabled());
    assert(Near(btn.GetCurrentScale(), 1.0f));

    // Default -> Pressed on touch down inside
    bool consumed = btn.OnTouchDown(1, 20.0f, 20.0f);
    assert(consumed);
    assert(btn.GetState() == UIButtonState::Pressed);
    assert(btn.IsPressed());

    // Update moves transition toward pressed (120-180ms ease-out target)
    btn.Update(0.08f);
    assert(btn.GetTransition() > 0.0f);
    assert(btn.GetCurrentScale() < 1.0f);
    assert(btn.GetCurrentScale() >= 0.96f - 0.001f);

    // Pressed -> Default on touch up inside (click fires)
    bool clicked = false;
    btn.SetOnClick([&]() { clicked = true; });
    // Need to re-press because we set onclick after; easier: press again
    btn.OnTouchDown(2, 20.0f, 20.0f);
    btn.Update(0.2f);
    assert(Near(btn.GetCurrentScale(), 0.96f, 0.01f)); // pressed scale spec
    bool up = btn.OnTouchUp(2, 20.0f, 20.0f);
    assert(up);
    assert(clicked);
    assert(btn.GetState() == UIButtonState::Default);

    // Disabled
    btn.SetEnabled(false);
    assert(btn.GetState() == UIButtonState::Disabled);
    assert(!btn.IsEnabled());
    assert(!btn.OnTouchDown(3, 20.0f, 20.0f)); // disabled ignores touch
    btn.SetEnabled(true);
    assert(btn.GetState() == UIButtonState::Default);

    // Focus
    btn.SetFocused(true);
    assert(btn.GetState() == UIButtonState::Focus);
    btn.SetFocused(false);
    assert(btn.GetState() == UIButtonState::Default);

    // Min touch target enforcement
    UIButton small;
    small.SetMinHeight(48.0f);
    small.SetRect(0, 0, 10, 10);
    assert(small.GetRect().w >= 48.0f);
    assert(small.GetRect().h >= 48.0f);

    std::cout << "[PASS] button state transitions\n";
}

static void testLayoutCalculation() {
    LoginScreen login;
    login.Initialize(nullptr);

    // 1280x720 reference
    const LoginLayout& l = login.ComputeLayout(1280.0f, 720.0f);
    assert(Near(l.screenW, 1280.0f) && Near(l.screenH, 720.0f));
    assert(l.panel.w > 0 && l.panel.h > 0);
    // Panel centered
    assert(Near(l.panel.x + l.panel.w * 0.5f, 640.0f, 1.0f));
    assert(Near(l.panel.y + l.panel.h * 0.5f, 360.0f, 1.0f));
    // Components inside panel
    assert(l.usernameField.x >= l.panel.x && l.usernameField.x + l.usernameField.w <= l.panel.x + l.panel.w + 1.0f);
    assert(l.passwordField.y > l.usernameField.y);
    assert(l.loginButton.w >= l.usernameField.w - 1.0f);
    assert(l.loginButton.h >= 56.0f);
    // Touch target min
    assert(l.loginButton.h >= 48.0f);
    assert(l.settingsButton.h >= 44.0f);
    // Settings/exit modal centered
    assert(Near(l.settingsPanel.x + l.settingsPanel.w * 0.5f, 640.0f, 1.0f));
    assert(Near(l.exitConfirmPanel.x + l.exitConfirmPanel.w * 0.5f, 640.0f, 1.0f));

    // Responsive: smaller screen
    const LoginLayout& s = login.ComputeLayout(720.0f, 1280.0f); // portrait-ish
    assert(s.panel.w <= 720.0f - 48.0f + 1.0f);
    assert(s.panel.x >= 0.0f);
    assert(s.panel.x + s.panel.w <= 720.0f + 1.0f);
    assert(s.loginButton.h >= 56.0f);

    // Very small screen triggers low-end fallback heuristic (no crash, layout still valid)
    const LoginLayout& tiny = login.ComputeLayout(640.0f, 360.0f);
    assert(tiny.panel.w > 0 && tiny.panel.h > 0);
    assert(tiny.loginButton.w > 0);

    std::cout << "[PASS] layout calculation\n";
}

static void testInputFieldFocusAndInput() {
    using namespace RANMobile;
    UIInputField f;
    f.SetRect(0, 0, 300, 56);
    f.SetLabel("USERNAME");
    assert(!f.IsFocused());
    assert(f.OnTouchDown(0, 10, 10));
    assert(f.IsFocused());
    f.OnTextInput("abc");
    assert(f.GetText() == "abc");
    f.OnBackspace();
    assert(f.GetText() == "ab");
    f.SetPasswordMode(true);
    f.SetText("secret");
    assert(f.GetDisplayText() == "******");
    std::cout << "[PASS] input field focus/input\n";
}

int main() {
    std::cout << "=== Phase 5 UI Unit Tests ===\n";
    testColorTokens();
    testButtonStateTransitions();
    testLayoutCalculation();
    testInputFieldFocusAndInput();
    std::cout << "ALL PHASE 5 UI TESTS: PASS\n";
    return 0;
}
