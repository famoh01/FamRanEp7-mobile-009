#pragma once

#include <cstdint>

namespace RANMobile {

struct UIColor {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    static constexpr UIColor RGBA(float r_, float g_, float b_, float a_ = 1.0f) {
        return {r_, g_, b_, a_};
    }
    static constexpr UIColor Hex(uint32_t rgb, float a_ = 1.0f) {
        return {
            static_cast<float>((rgb >> 16) & 0xFF) / 255.0f,
            static_cast<float>((rgb >> 8) & 0xFF) / 255.0f,
            static_cast<float>(rgb & 0xFF) / 255.0f,
            a_
        };
    }
    UIColor WithAlpha(float a_) const { return {r, g, b, a_}; }
    UIColor Scale(float s) const { return {r * s, g * s, b * s, a}; }
};

struct UIRect {
    float x = 0, y = 0, w = 0, h = 0;
    bool Contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

// Phase 5 "Modern Fantasy Luxury" design tokens — central source of truth.
// Do not hardcode these values in individual components; consume UITheme.
struct UITheme {
    // Background
    UIColor bgTop    = UIColor::Hex(0x0B0E14);
    UIColor bgBottom = UIColor::Hex(0x1A1F2E);

    // Accent Primary Gold (main buttons / highlights)
    UIColor goldStart = UIColor::Hex(0xD4AF37);
    UIColor goldEnd   = UIColor::Hex(0xF2D88F);

    // Accent Secondary Blue
    UIColor blueStart = UIColor::Hex(0x3A5FCD);
    UIColor blueEnd   = UIColor::Hex(0x1E3A8A);

    // Glass panel
    UIColor glassFill   = UIColor::RGBA(1.0f, 1.0f, 1.0f, 0.06f);
    float   glassBlurPx = 12.0f; // visual target; actual blur is quality-dependent

    // Border / glow
    UIColor borderGlow      = UIColor::Hex(0xFFD700, 0.40f);
    float   borderWidthPx   = 1.5f;
    float   borderGlowMinPx = 4.0f;
    float   borderGlowMaxPx = 8.0f;

    // Text
    UIColor textPrimary   = UIColor::Hex(0xF5F5F0);
    UIColor textSecondary = UIColor::Hex(0x8A8D98);

    // Status
    UIColor errorColor   = UIColor::Hex(0xFF5C5C);
    UIColor successColor = UIColor::Hex(0x4ADE80);

    // Typography (sp targets; renderer uses scale relative to base font px)
    float titleSp   = 28.0f;
    float bodySp    = 16.0f;
    float captionSp = 12.0f;

    // Component metrics
    float minTouchTargetDp = 48.0f;
    float buttonMinHeightDp = 48.0f;
    float loginButtonHeightDp = 56.0f;
    float cornerRadiusDp = 14.0f;
    float transitionFastMs = 120.0f;
    float transitionNormalMs = 180.0f;
    float screenTransitionMs = 280.0f;
};

inline const UITheme& GetDefaultUITheme() {
    static UITheme kTheme;
    return kTheme;
}

} // namespace RANMobile
