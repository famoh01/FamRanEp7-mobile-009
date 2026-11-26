#pragma once

#include <string>
#include "../UITheme.h"

namespace RANMobile {

class MobileRenderer;

class UIInputField {
public:
    UIInputField();
    ~UIInputField();

    void SetRect(float x, float y, float w, float h);
    void SetLabel(const std::string& label);
    void SetPlaceholder(const std::string& ph);
    void SetText(const std::string& text);
    void SetPasswordMode(bool pw) { m_passwordMode = pw; }
    void SetTheme(const UITheme* theme) { m_theme = theme; }
    void SetFocused(bool focused) { m_focused = focused; }
    void SetMaxLength(size_t n) { m_maxLength = n; }

    const std::string& GetText() const { return m_text; }
    const std::string& GetLabel() const { return m_label; }
    bool IsFocused() const { return m_focused; }
    bool IsPasswordMode() const { return m_passwordMode; }
    const UIRect& GetRect() const { return m_rect; }
    std::string GetDisplayText() const;

    bool OnTouchDown(int pointerId, float x, float y);
    void OnTextInput(const std::string& utf8);
    void OnBackspace();
    void Update(float dt);
    void Draw(MobileRenderer* renderer) const;

private:
    UIRect m_rect;
    std::string m_label;
    std::string m_placeholder;
    std::string m_text;
    bool m_focused = false;
    bool m_passwordMode = false;
    size_t m_maxLength = 64;
    float m_focusAnim = 0.0f; // 0..1
    float m_cursorBlink = 0.0f;
    const UITheme* m_theme = nullptr;
};

} // namespace RANMobile
