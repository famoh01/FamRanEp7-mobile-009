#include "UIButton.h"
#include "../../Engine/Renderer/MobileRenderer.h"
#include <algorithm>
#include <cmath>

namespace RANMobile {

UIButton::UIButton() = default;
UIButton::~UIButton() = default;

void UIButton::SetRect(float x, float y, float w, float h) {
    m_rect = {x, y, std::max(w, 48.0f), std::max(h, m_minHeight)};
}
void UIButton::SetLabel(const std::string& label) { m_label = label; }
void UIButton::SetTheme(const UITheme* theme) { m_theme = theme; if (theme) m_transitionMs = theme->transitionNormalMs; }
void UIButton::SetEnabled(bool enabled) {
    if (!enabled) { m_state = UIButtonState::Disabled; m_pressedPointer = -1; m_transition = 0.0f; }
    else if (m_state == UIButtonState::Disabled) { m_state = UIButtonState::Default; }
}
void UIButton::SetMinHeight(float h) { m_minHeight = std::max(48.0f, h); if (m_rect.h < m_minHeight) m_rect.h = m_minHeight; }

float UIButton::GetCurrentScale() const {
    // Pressed: scale 0.96 (spec). Ease-out transition.
    return 1.0f - 0.04f * m_transition;
}

void UIButton::SetFocused(bool focused) {
    if (m_state == UIButtonState::Disabled || m_state == UIButtonState::Pressed) return;
    m_state = focused ? UIButtonState::Focus : UIButtonState::Default;
}

bool UIButton::OnTouchDown(int pointerId, float x, float y) {
    if (!m_visible || m_state == UIButtonState::Disabled) return false;
    if (!m_rect.Contains(x, y)) return false;
    m_state = UIButtonState::Pressed;
    m_pressedPointer = pointerId;
    return true;
}

bool UIButton::OnTouchMove(int pointerId, float x, float y) {
    if (m_pressedPointer != pointerId || m_state == UIButtonState::Disabled) return false;
    // If finger slides out, keep pressed visually but cancel on up? Common mobile behavior:
    // keep pressed state while tracking; if up outside, don't click.
    (void)x; (void)y;
    return true;
}

bool UIButton::OnTouchUp(int pointerId, float x, float y) {
    if (m_pressedPointer != pointerId) return false;
    bool inside = m_rect.Contains(x, y);
    bool wasPressed = (m_state == UIButtonState::Pressed);
    m_pressedPointer = -1;
    if (m_state != UIButtonState::Disabled) m_state = UIButtonState::Default;
    if (wasPressed && inside && m_onClick) m_onClick();
    return wasPressed;
}

void UIButton::Update(float dt) {
    float target = (m_state == UIButtonState::Pressed) ? 1.0f : 0.0f;
    float dur = std::max(0.001f, m_transitionMs / 1000.0f);
    // ease-out: fast approach
    float speed = 1.0f / dur;
    if (m_transition < target) m_transition = std::min(target, m_transition + speed * dt * 1.5f);
    else if (m_transition > target) m_transition = std::max(target, m_transition - speed * dt);
}

void UIButton::Draw(MobileRenderer* renderer) const {
    if (!m_visible || !renderer) return;
    const UITheme& t = m_theme ? *m_theme : GetDefaultUITheme();
    float scale = GetCurrentScale();
    float cx = m_rect.x + m_rect.w * 0.5f;
    float cy = m_rect.y + m_rect.h * 0.5f;
    float w = m_rect.w * scale;
    float h = m_rect.h * scale;
    float x = cx - w * 0.5f;
    float y = cy - h * 0.5f;
    float radius = t.cornerRadiusDp;

    // Pressed brightness -15%
    float bright = 1.0f - 0.15f * m_transition;
    float alpha = (m_state == UIButtonState::Disabled) ? 0.5f : 1.0f;
    float satMul = (m_state == UIButtonState::Disabled) ? 0.3f : 1.0f; // desaturate 70%

    auto desat = [satMul](UIColor c) {
        float lum = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
        return UIColor{lum + (c.r - lum) * satMul, lum + (c.g - lum) * satMul, lum + (c.b - lum) * satMul, c.a};
    };

    // Shadow (soft drop-shadow, offset-Y 2-4px, blur 6-10px, opacity 20-30%)
    if (m_state != UIButtonState::Disabled) {
        renderer->DrawUIRoundedShadow(x, y, w, h, radius, 0.0f, 3.0f * scale, 8.0f, 0.0f, 0.0f, 0.0f, 0.25f);
    }

    if (m_primary) {
        UIColor c1 = desat(t.goldStart.Scale(bright)).WithAlpha(alpha);
        UIColor c2 = desat(t.goldEnd.Scale(bright)).WithAlpha(alpha);
        renderer->DrawUIGradientRoundedRect(x, y, w, h, radius, c1.r, c1.g, c1.b, c1.a, c2.r, c2.g, c2.b, c2.a, true);
        // Border glow (subtle)
        UIColor glow = t.borderGlow;
        float glowA = (m_state == UIButtonState::Focus ? 0.6f : 0.25f) * alpha;
        renderer->DrawUIRoundedGlow(x, y, w, h, radius, t.borderGlowMinPx, glow.r, glow.g, glow.b, glowA);
        renderer->DrawUIRoundedBorder(x, y, w, h, radius, t.borderWidthPx, glow.r, glow.g, glow.b, 0.4f * alpha);
    } else {
        // Outline/text button style
        UIColor fill = t.glassFill.WithAlpha(0.04f * alpha);
        renderer->DrawUIRoundedRect(x, y, w, h, radius, fill.r, fill.g, fill.b, fill.a);
        renderer->DrawUIRoundedBorder(x, y, w, h, radius, 1.0f, t.goldStart.r, t.goldStart.g, t.goldStart.b, 0.35f * alpha);
    }

    // Focus outer ring
    if (m_state == UIButtonState::Focus) {
        renderer->DrawUIRoundedBorder(x - 2.0f, y - 2.0f, w + 4.0f, h + 4.0f, radius + 2.0f, 2.0f,
                                       t.goldEnd.r, t.goldEnd.g, t.goldEnd.b, 0.7f);
    }

    // Label (centered)
    if (!m_label.empty()) {
        float textScale = 0.75f; // ~21px at base 28px, body/title balance
        float tw = renderer->MeasureTextWidth(m_label, textScale);
        float tx = x + (w - tw) * 0.5f;
        float ty = y + (h - t.bodySp * textScale) * 0.5f - 2.0f;
        UIColor tc = m_primary ? UIColor::Hex(0x1A1200, alpha) : t.textPrimary.WithAlpha(alpha);
        renderer->DrawUIText(tx, ty, m_label, textScale, tc.r, tc.g, tc.b, tc.a);
    }
}

} // namespace RANMobile
