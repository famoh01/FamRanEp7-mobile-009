#include "UIInputField.h"
#include "../../Engine/Renderer/MobileRenderer.h"
#include <algorithm>

namespace RANMobile {

UIInputField::UIInputField() = default;
UIInputField::~UIInputField() = default;

void UIInputField::SetRect(float x, float y, float w, float h) {
    m_rect = {x, y, std::max(w, 48.0f), std::max(h, 48.0f)};
}
void UIInputField::SetLabel(const std::string& label) { m_label = label; }
void UIInputField::SetPlaceholder(const std::string& ph) { m_placeholder = ph; }
void UIInputField::SetText(const std::string& text) { m_text = text.substr(0, m_maxLength); }

std::string UIInputField::GetDisplayText() const {
    if (m_passwordMode) return std::string(m_text.size(), '*');
    return m_text;
}

bool UIInputField::OnTouchDown(int /*pointerId*/, float x, float y) {
    if (m_rect.Contains(x, y)) {
        m_focused = true;
        return true;
    }
    return false;
}

void UIInputField::OnTextInput(const std::string& utf8) {
    if (!m_focused || utf8.empty()) return;
    // Append UTF-8 bytes; limit by byte count as simple guard (keeps existing behavior reasonable)
    if (m_text.size() + utf8.size() <= m_maxLength) {
        m_text += utf8;
    }
}

void UIInputField::OnBackspace() {
    if (!m_focused || m_text.empty()) return;
    // Pop back one UTF-8 codepoint
    size_t i = m_text.size() - 1;
    while (i > 0 && (static_cast<unsigned char>(m_text[i]) & 0xC0) == 0x80) --i;
    m_text.erase(i);
}

void UIInputField::Update(float dt) {
    float target = m_focused ? 1.0f : 0.0f;
    float speed = 6.0f; // ~150ms feel
    if (m_focusAnim < target) m_focusAnim = std::min(target, m_focusAnim + speed * dt);
    else if (m_focusAnim > target) m_focusAnim = std::max(target, m_focusAnim - speed * dt);
    m_cursorBlink += dt;
    if (m_cursorBlink > 1.0f) m_cursorBlink -= 1.0f;
}

void UIInputField::Draw(MobileRenderer* renderer) const {
    if (!renderer) return;
    const UITheme& t = m_theme ? *m_theme : GetDefaultUITheme();
    float x = m_rect.x, y = m_rect.y, w = m_rect.w, h = m_rect.h;
    float radius = 10.0f;

    // Background
    UIColor bg = UIColor::RGBA(1.0f, 1.0f, 1.0f, 0.04f + 0.02f * m_focusAnim);
    renderer->DrawUIRoundedRect(x, y, w, h, radius, bg.r, bg.g, bg.b, bg.a);

    // Border / focus glow (gold on focus)
    UIColor border = t.textSecondary.WithAlpha(0.35f);
    UIColor focusC = t.goldStart;
    float br = border.r + (focusC.r - border.r) * m_focusAnim;
    float bgc = border.g + (focusC.g - border.g) * m_focusAnim;
    float bb = border.b + (focusC.b - border.b) * m_focusAnim;
    float ba = border.a + (0.85f - border.a) * m_focusAnim;
    renderer->DrawUIRoundedBorder(x, y, w, h, radius, 1.0f + 0.5f * m_focusAnim, br, bgc, bb, ba);
    if (m_focusAnim > 0.01f) {
        renderer->DrawUIRoundedGlow(x, y, w, h, radius, 6.0f, t.goldEnd.r, t.goldEnd.g, t.goldEnd.b, 0.18f * m_focusAnim);
    }

    // Floating label
    bool floated = m_focused || !m_text.empty();
    float labelScale = floated ? 0.55f : 0.72f;
    float labelY = floated ? (y + 6.0f) : (y + (h - 16.0f) * 0.5f);
    UIColor lc = floated ? t.goldEnd.WithAlpha(0.85f) : t.textSecondary.WithAlpha(0.8f);
    if (!m_label.empty()) {
        renderer->DrawUIText(x + 14.0f, labelY, m_label, labelScale, lc.r, lc.g, lc.b, lc.a);
    }

    // Text / placeholder
    std::string disp = GetDisplayText();
    float textY = y + h * 0.52f;
    float textScale = 0.72f;
    if (disp.empty() && !m_focused && !m_placeholder.empty()) {
        renderer->DrawUIText(x + 14.0f, textY, m_placeholder, textScale, t.textSecondary.r, t.textSecondary.g, t.textSecondary.b, 0.45f);
    } else if (!disp.empty()) {
        renderer->DrawUIText(x + 14.0f, textY, disp, textScale, t.textPrimary.r, t.textPrimary.g, t.textPrimary.b, 1.0f);
    }

    // Cursor
    if (m_focused && m_cursorBlink < 0.5f) {
        float tw = renderer->MeasureTextWidth(disp, textScale);
        renderer->DrawUIQuad(x + 14.0f + tw + 2.0f, textY + 1.0f, 2.0f, 18.0f * textScale + 6.0f, t.goldEnd.r, t.goldEnd.g, t.goldEnd.b, 0.9f);
    }
}

} // namespace RANMobile
