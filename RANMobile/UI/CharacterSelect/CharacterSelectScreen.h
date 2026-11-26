#pragma once

#include <string>
#include <vector>
#include "../UITheme.h"
#include "../Components/UIButton.h"
#include "../../Client/Character/CharacterSelectManager.h"

namespace RANMobile {
class NetworkClient;
class MobileRenderer;

struct CharacterSelectLayout {
    float screenW = 1280.0f;
    float screenH = 720.0f;
    UIRect panel;
    UIRect titleArea;
    UIRect slotCards[3]; // up to 3 visible slots
    UIRect selectButton;
    UIRect backButton;
};

// Phase 5 extension: Character Select UI shell (procedural/vector).
// Data fields bind to EP7CharacterSlot (charId, name, level, charClass) from protocol struct.
// Mockup/test values must use [SLOT_EMPTY] / TEST_SLOT_A style only - no real game names.
class CharacterSelectScreen {
public:
    void Initialize(NetworkClient* net, CharacterSelectManager* manager);

    void SetScreenSize(float w, float h);
    const CharacterSelectLayout& ComputeLayout(float w, float h);
    const CharacterSelectLayout& GetLayout() const { return m_layout; }

    void HandleTouchDown(int pointerId, float x, float y);
    void HandleTouchMove(int pointerId, float x, float y);
    void HandleTouchUp(int pointerId, float x, float y);
    void Update(float dt);
    void Draw(MobileRenderer* renderer);

    bool WantsBack() const { return m_wantsBack; }
    void ClearWantsBack() { m_wantsBack = false; }
    bool WantsStartGame() const { return m_wantsStartGame; }
    void ClearWantsStartGame() { m_wantsStartGame = false; }

    // Test/helper: seed mock slots (clearly labeled, not real game data)
    void SeedMockSlotsForTest();

    static std::string ClassLabel(uint16_t charClass);

private:
    void ApplyLayout();
    void DrawBackground(MobileRenderer* r);
    void DrawPanel(MobileRenderer* r);
    void DrawSlotCard(MobileRenderer* r, int slotIndex, const UIRect& rect);

    NetworkClient* m_net = nullptr; // non-owning
    CharacterSelectManager* m_manager = nullptr; // non-owning
    UITheme m_theme;
    CharacterSelectLayout m_layout;
    UIButton m_selectButton;
    UIButton m_backButton;
    bool m_wantsBack = false;
    bool m_wantsStartGame = false;
    float m_animTime = 0.0f;
    float m_panelAnim = 0.0f;
};

} // namespace RANMobile
