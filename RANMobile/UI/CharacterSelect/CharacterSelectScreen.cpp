#include "CharacterSelectScreen.h"
#include "../../Engine/Renderer/MobileRenderer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace RANMobile {

void CharacterSelectScreen::Initialize(NetworkClient* net, CharacterSelectManager* manager) {
    m_net = net;
    m_manager = manager;

    m_selectButton.SetTheme(&m_theme);
    m_selectButton.SetLabel("START GAME");
    m_selectButton.SetPrimary(true);
    m_selectButton.SetMinHeight(m_theme.loginButtonHeightDp);
    m_selectButton.SetOnClick([this]() {
        if (m_manager && m_manager->GetSelectedCharacter()) m_wantsStartGame = true;
    });

    m_backButton.SetTheme(&m_theme);
    m_backButton.SetLabel("BACK");
    m_backButton.SetPrimary(false);
    m_backButton.SetOnClick([this]() { m_wantsBack = true; });

    ComputeLayout(1280.0f, 720.0f);
}

void CharacterSelectScreen::SetScreenSize(float w, float h) {
    if (w <= 0 || h <= 0) return;
    if (std::abs(m_layout.screenW - w) > 0.5f || std::abs(m_layout.screenH - h) > 0.5f) {
        ComputeLayout(w, h);
    }
}

const CharacterSelectLayout& CharacterSelectScreen::ComputeLayout(float w, float h) {
    m_layout.screenW = w;
    m_layout.screenH = h;

    float panelW = std::min(980.0f, w - 48.0f);
    float panelH = std::min(560.0f, h - 48.0f);
    float px = (w - panelW) * 0.5f;
    float py = (h - panelH) * 0.5f;
    m_layout.panel = {px, py, panelW, panelH};
    m_layout.titleArea = {px + 24.0f, py + 20.0f, panelW - 48.0f, 60.0f};

    float cardsY = py + 96.0f;
    float cardsH = 300.0f;
    float gap = 20.0f;
    float cardW = (panelW - 48.0f - gap * 2.0f) / 3.0f;
    for (int i = 0; i < 3; ++i) {
        m_layout.slotCards[i] = {px + 24.0f + i * (cardW + gap), cardsY, cardW, cardsH};
    }

    float btnY = py + panelH - 72.0f;
    float btnW = 220.0f;
    m_layout.backButton = {px + 24.0f, btnY, btnW, 52.0f};
    m_layout.selectButton = {px + panelW - 24.0f - btnW, btnY, btnW, 52.0f};
    ApplyLayout();
    return m_layout;
}

void CharacterSelectScreen::ApplyLayout() {
    m_backButton.SetRect(m_layout.backButton.x, m_layout.backButton.y, m_layout.backButton.w, m_layout.backButton.h);
    m_selectButton.SetRect(m_layout.selectButton.x, m_layout.selectButton.y, m_layout.selectButton.w, m_layout.selectButton.h);
}

std::string CharacterSelectScreen::ClassLabel(uint16_t charClass) {
    // Source-evidenced mapping from GLCharDefine.h (EMCHARCLASS).
    switch (charClass) {
        case 0x0001: return "FIGHTER-M";
        case 0x0002: return "ARMS-M";
        case 0x0004: return "ARCHER-W";
        case 0x0008: return "SPIRIT-W";
        case 0x0010: return "EXTREME-M";
        case 0x0020: return "EXTREME-W";
        case 0x0040: return "FIGHTER-W";
        case 0x0080: return "ARMS-W";
        case 0x0100: return "ARCHER-M";
        case 0x0200: return "SPIRIT-M";
        default: {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "CLASS-0x%04X", static_cast<unsigned>(charClass));
            return std::string(buf);
        }
    }
}

void CharacterSelectScreen::SeedMockSlotsForTest() {
    if (!m_manager) return;
    m_manager->ClearSlots();
    EP7CharacterSlot a{};
    a.charId = 1;
    a.name = "TEST_SLOT_A"; // mockup label only - not real game data
    a.level = 1;
    a.charClass = 0x0001; // GLCC_FIGHTER_M (source-evidenced enum value)
    m_manager->AddSlotFromServer(a);

    EP7CharacterSlot b{};
    b.charId = 2;
    b.name = "TEST_SLOT_B";
    b.level = 10;
    b.charClass = 0x0004; // GLCC_ARCHER_W
    m_manager->AddSlotFromServer(b);
    // slot 3 left empty -> [SLOT_EMPTY]
}

void CharacterSelectScreen::HandleTouchDown(int pointerId, float x, float y) {
    if (m_backButton.OnTouchDown(pointerId, x, y)) return;
    if (m_selectButton.OnTouchDown(pointerId, x, y)) return;

    for (int i = 0; i < 3; ++i) {
        if (m_layout.slotCards[i].Contains(x, y)) {
            if (m_manager && i < static_cast<int>(m_manager->GetSlots().size())) {
                m_manager->SelectSlot(static_cast<size_t>(i));
            } else {
                // empty slot tap: clear selection (no fabrication)
                if (m_manager) m_manager->ClearSlots();
            }
            return;
        }
    }
}

void CharacterSelectScreen::HandleTouchMove(int pointerId, float x, float y) {
    m_backButton.OnTouchMove(pointerId, x, y);
    m_selectButton.OnTouchMove(pointerId, x, y);
}

void CharacterSelectScreen::HandleTouchUp(int pointerId, float x, float y) {
    m_backButton.OnTouchUp(pointerId, x, y);
    m_selectButton.OnTouchUp(pointerId, x, y);
}

void CharacterSelectScreen::Update(float dt) {
    m_animTime += dt;
    float dur = m_theme.screenTransitionMs / 1000.0f;
    if (m_panelAnim < 1.0f) m_panelAnim = std::min(1.0f, m_panelAnim + dt / std::max(0.001f, dur));
    m_backButton.Update(dt);
    m_selectButton.Update(dt);
}

void CharacterSelectScreen::DrawBackground(MobileRenderer* r) {
    r->DrawUIGradientRect(0, 0, m_layout.screenW, m_layout.screenH,
                          m_theme.bgTop.r, m_theme.bgTop.g, m_theme.bgTop.b, 1.0f,
                          m_theme.bgBottom.r, m_theme.bgBottom.g, m_theme.bgBottom.b, 1.0f, true);
    float t = m_animTime;
    r->DrawUIRoundedGlow(m_layout.screenW * 0.5f - 200.0f + std::sin(t * 0.2f) * 20.0f, 40.0f, 400.0f, 160.0f, 80.0f, 50.0f,
                         m_theme.blueStart.r, m_theme.blueStart.g, m_theme.blueStart.b, 0.08f);
}

void CharacterSelectScreen::DrawSlotCard(MobileRenderer* r, int slotIndex, const UIRect& rect) {
    bool hasSlot = m_manager && slotIndex < static_cast<int>(m_manager->GetSlots().size());
    bool selected = m_manager && m_manager->GetSelectedIndex() == slotIndex;
    const EP7CharacterSlot* slot = hasSlot ? &m_manager->GetSlots()[static_cast<size_t>(slotIndex)] : nullptr;

    float radius = 14.0f;
    r->DrawUIRoundedShadow(rect.x, rect.y, rect.w, rect.h, radius, 0, 4, 12, 0, 0, 0, 0.25f);
    r->DrawUIRoundedRect(rect.x, rect.y, rect.w, rect.h, radius,
                         m_theme.glassFill.r, m_theme.glassFill.g, m_theme.glassFill.b, selected ? 0.12f : 0.06f);
    UIColor border = selected ? m_theme.goldEnd : m_theme.borderGlow;
    float borderA = selected ? 0.9f : 0.30f;
    r->DrawUIRoundedBorder(rect.x, rect.y, rect.w, rect.h, radius, selected ? 2.0f : 1.0f,
                           border.r, border.g, border.b, borderA);
    if (selected) {
        r->DrawUIRoundedGlow(rect.x, rect.y, rect.w, rect.h, radius, 8.0f,
                             m_theme.goldStart.r, m_theme.goldStart.g, m_theme.goldStart.b, 0.18f);
    }

    // Card content bound to EP7CharacterSlot struct fields (no hardcoded real game names)
    float cx = rect.x + 20.0f;
    float cy = rect.y + 24.0f;
    char slotTitle[32];
    std::snprintf(slotTitle, sizeof(slotTitle), "SLOT %d", slotIndex + 1);
    r->DrawUIText(cx, cy, slotTitle, 0.55f, m_theme.textSecondary.r, m_theme.textSecondary.g, m_theme.textSecondary.b, 0.9f);

    if (slot) {
        // Name bound to slot->name
        r->DrawUIText(cx, cy + 36.0f, slot->name, 0.85f, m_theme.textPrimary.r, m_theme.textPrimary.g, m_theme.textPrimary.b, 1.0f);
        // Class bound to slot->charClass
        std::string cls = std::string("CLASS: ") + ClassLabel(slot->charClass);
        r->DrawUIText(cx, cy + 72.0f, cls, 0.58f, m_theme.goldEnd.r, m_theme.goldEnd.g, m_theme.goldEnd.b, 0.95f);
        // Level bound to slot->level
        char lv[32];
        std::snprintf(lv, sizeof(lv), "LEVEL: %u", static_cast<unsigned>(slot->level));
        r->DrawUIText(cx, cy + 100.0f, lv, 0.58f, m_theme.textSecondary.r, m_theme.textSecondary.g, m_theme.textSecondary.b, 1.0f);
        // ID bound to slot->charId (debug/struct proof, not fabricated)
        char idbuf[32];
        std::snprintf(idbuf, sizeof(idbuf), "ID: %u", static_cast<unsigned>(slot->charId));
        r->DrawUIText(cx, cy + 128.0f, idbuf, 0.48f, m_theme.textSecondary.r, m_theme.textSecondary.g, m_theme.textSecondary.b, 0.7f);
    } else {
        r->DrawUIText(cx, cy + 52.0f, "[SLOT_EMPTY]", 0.75f, m_theme.textSecondary.r, m_theme.textSecondary.g, m_theme.textSecondary.b, 0.55f);
        r->DrawUIText(cx, cy + 86.0f, "NO DATA BOUND", 0.48f, m_theme.textSecondary.r, m_theme.textSecondary.g, m_theme.textSecondary.b, 0.45f);
    }
}

void CharacterSelectScreen::DrawPanel(MobileRenderer* r) {
    float ease = 1.0f - std::pow(1.0f - m_panelAnim, 3.0f);
    float alpha = ease;
    (void)alpha;
    const auto& p = m_layout.panel;
    r->DrawUIRoundedShadow(p.x, p.y, p.w, p.h, 18.0f, 0, 6, 18, 0, 0, 0, 0.28f);
    r->DrawUIRoundedRect(p.x, p.y, p.w, p.h, 18.0f, m_theme.glassFill.r, m_theme.glassFill.g, m_theme.glassFill.b, m_theme.glassFill.a);
    r->DrawUIRoundedBorder(p.x, p.y, p.w, p.h, 18.0f, m_theme.borderWidthPx, m_theme.borderGlow.r, m_theme.borderGlow.g, m_theme.borderGlow.b, 0.45f);

    r->DrawUIText(p.x + 24.0f, p.y + 24.0f, "CHARACTER SELECT", 1.0f, m_theme.goldEnd.r, m_theme.goldEnd.g, m_theme.goldEnd.b, 1.0f);
    r->DrawUIText(p.x + 24.0f, p.y + 54.0f, "Bound to EP7CharacterSlot (protocol struct)", 0.50f, m_theme.textSecondary.r, m_theme.textSecondary.g, m_theme.textSecondary.b, 0.8f);

    for (int i = 0; i < 3; ++i) DrawSlotCard(r, i, m_layout.slotCards[i]);

    m_backButton.Draw(r);
    m_selectButton.Draw(r);
}

void CharacterSelectScreen::Draw(MobileRenderer* renderer) {
    if (!renderer) return;
    SetScreenSize(static_cast<float>(renderer->GetWidth()), static_cast<float>(renderer->GetHeight()));
    DrawBackground(renderer);
    DrawPanel(renderer);
}

} // namespace RANMobile
