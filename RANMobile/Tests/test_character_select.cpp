#include "../UI/CharacterSelect/CharacterSelectScreen.h"
#include "../Client/Character/CharacterSelectManager.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace RANMobile;
    CharacterSelectManager mgr;
    CharacterSelectScreen screen;
    screen.Initialize(nullptr, &mgr);
    screen.ComputeLayout(1280, 720);

    // Empty slots -> [SLOT_EMPTY] display state (no real game names)
    assert(mgr.GetSlots().empty());
    auto l = screen.GetLayout();
    assert(l.panel.w > 0 && l.slotCards[0].w > 0);

    // Seed mock slots (TEST_SLOT_A/B only)
    screen.SeedMockSlotsForTest();
    assert(mgr.GetSlots().size() == 2);
    assert(mgr.GetSlots()[0].name == "TEST_SLOT_A");
    assert(mgr.GetSlots()[0].level == 1);
    assert(mgr.GetSlots()[0].charClass == 0x0001);
    assert(mgr.GetSlots()[1].name == "TEST_SLOT_B");

    // Class label mapping is source-evidenced (GLCharDefine.h EMCHARCLASS)
    assert(CharacterSelectScreen::ClassLabel(0x0001) == "FIGHTER-M");
    assert(CharacterSelectScreen::ClassLabel(0x0004) == "ARCHER-W");

    // Selection binds to struct
    auto c = screen.GetLayout().slotCards[0];
    screen.HandleTouchDown(0, c.x + c.w*0.5f, c.y + c.h*0.5f);
    screen.HandleTouchUp(0, c.x + c.w*0.5f, c.y + c.h*0.5f);
    assert(mgr.GetSelectedIndex() == 0);
    assert(mgr.GetSelectedCharacter() != nullptr);
    assert(mgr.GetSelectedCharacter()->name == "TEST_SLOT_A");

    std::cout << "ALL CHARACTER SELECT TESTS: PASS\n";
    return 0;
}
