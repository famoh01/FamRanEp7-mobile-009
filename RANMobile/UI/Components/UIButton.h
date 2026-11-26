#pragma once

#include <functional>
#include <string>
#include "../UITheme.h"

namespace RANMobile {

class MobileRenderer;

enum class UIButtonState {
    Default,
    Pressed,
    Disabled,
    Focus
};

// Reusable button component (Phase 5 Modern Fantasy Luxury).
// 4 states: Default/Pressed/Disabled/Focus with 120-180ms ease-out transitions.
// Min touch target 48dp; pressed scale 0.96; brightness -15%; shadow + gold gradient.
class UIButton {
public:
    UIButton();
    ~UIButton();

    void SetRect(float x, float y, float w, float h);
    void SetLabel(const std::string& label);
    void SetTheme(const UITheme* theme);
    void SetEnabled(bool enabled);
    void SetVisible(bool visible) { m_visible = visible; }
    void SetOnClick(std::function<void()> cb) { m_onClick = std::move(cb); }
    void SetPrimary(bool primary) { m_primary = primary; } // gold gradient vs outline/text
    void SetMinHeight(float h);

    const UIRect& GetRect() const { return m_rect; }
    UIButtonState GetState() const { return m_state; }
    bool IsEnabled() const { return m_state != UIButtonState::Disabled; }
    bool IsVisible() const { return m_visible; }
    bool IsPressed() const { return m_state == UIButtonState::Pressed; }
    float GetTransition() const { return m_transition; } // 0..1 toward pressed
    float GetCurrentScale() const;

    // Touch handling returns true if consumed.
    bool OnTouchDown(int pointerId, float x, float y);
    bool OnTouchMove(int pointerId, float x, float y);
    bool OnTouchUp(int pointerId, float x, float y);

    void Update(float dt);
    void Draw(MobileRenderer* renderer) const;

    // For tests: simulate focus
    void SetFocused(bool focused);

private:
    UIRect m_rect;
    std::string m_label;
    const UITheme* m_theme = nullptr;
    UIButtonState m_state = UIButtonState::Default;
    bool m_visible = true;
    bool m_primary = true;
    float m_minHeight = 48.0f;
    float m_transition = 0.0f; // 0 default, 1 pressed
    float m_transitionMs = 150.0f;
    int m_pressedPointer = -1;
    std::function<void()> m_onClick;
};

} // namespace RANMobile
