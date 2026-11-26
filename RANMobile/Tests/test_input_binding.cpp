// Phase 3.2 Input Binding Vertical Slice — offline unit tests.
// Proves: touch/joystick axis -> movement intent -> SNETPC_GOTO packet (evidence-backed).
// No live server, no assets, no invented opcodes.

#include "../Client/Input/InputStateMachine.h"
#include "../Client/Input/MovementInputController.h"
#include "../Engine/Network/EP7Protocol.h"
#include "../UI/Joystick/VirtualJoystick.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

using namespace RANMobile;

static bool near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) <= eps; }

static void test_state_machine_extensible() {
    InputStateMachine sm;
    assert(sm.GetState() == InputState::Idle);
    sm.OnIntent(InputIntentType::Movement);
    assert(sm.GetState() == InputState::Moving);
    sm.OnIntent(InputIntentType::None);
    assert(sm.GetState() == InputState::Idle);
    // Extensibility placeholders (no UI impl required now)
    sm.OnIntent(InputIntentType::Skill);
    assert(sm.GetState() == InputState::CastingSkill);
    sm.OnIntent(InputIntentType::Inventory);
    assert(sm.GetState() == InputState::InventoryOpen);
    sm.Reset();
    assert(sm.GetState() == InputState::Idle);
    std::cout << "[PASS] InputStateMachine transitions/extensibility\n";
}

static void test_goto_packet_layout_evidence() {
    static_assert(sizeof(EP7SNetPCGoto) == 36, "SNETPC_GOTO size");
    static_assert(sizeof(EP7SNetPCMoveState) == 12, "SNETPC_MOVESTATE size");
    static_assert(EP7_NET_MSG_GCTRL == 2889, "GCTRL base");
    static_assert(EP7_NET_MSG_GCTRL_GOTO == 3031, "GOTO opcode");
    static_assert(EP7_NET_MSG_GCTRL_MOVESTATE == 3029, "MOVESTATE opcode");
    EP7SNetPCGoto pkt;
    assert(pkt.nmg.dwSize == 36);
    assert(pkt.nmg.nType == EP7_NET_MSG_GCTRL_GOTO);
    std::cout << "[PASS] SNETPC_GOTO layout/opcode evidence\n";
}

static void test_joystick_to_movement_intent() {
    MovementInputController ctrl;
    // No movement: magnitude below deadzone
    ctrl.Update(0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 10.f, 0.f, 20.f, 4.f, 8.f);
    assert(!ctrl.GetIntent().active);
    assert(ctrl.GetStateMachine().GetState() == InputState::Idle);
    EP7SNetPCGoto pkt;
    assert(ctrl.BuildGotoPacket(pkt) == false);

    // Move forward (axisY negative = up on screen in joystick convention? We follow
    // CharacterController: worldX = ax*cos - ay*sin, worldZ = ax*sin + ay*cos)
    // With yaw=0, axisX=1, axisY=0 => worldX=1, worldZ=0
    ctrl.Update(0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.f, 0.f, 0.f, 4.f, 8.f);
    const auto& it = ctrl.GetIntent();
    assert(it.active);
    assert(it.type == InputIntentType::Movement);
    assert(it.dwActState == EP7_EM_ACT_RUN); // magnitude 1.0 > 0.65
    assert(near(it.curPos.x, 0.f) && near(it.curPos.y, 0.f) && near(it.curPos.z, 0.f));
    assert(near(it.tarPos.x, 4.0f)); // runSpeed 8 * 0.5s = 4
    assert(near(it.tarPos.z, 0.0f));
    assert(ctrl.GetStateMachine().GetState() == InputState::Moving);
    assert(ctrl.BuildGotoPacket(pkt));
    assert(pkt.nmg.dwSize == 36);
    assert(pkt.nmg.nType == EP7_NET_MSG_GCTRL_GOTO);
    assert(pkt.dwActState == EP7_EM_ACT_RUN);
    assert(near(pkt.vCurPos.x, 0.f));
    assert(near(pkt.vTarPos.x, 4.0f));

    // Walk (magnitude <= 0.65)
    ctrl.Update(1.0f, 0.0f, 1.0f, 0.5f, 0.0f, 0.f, 0.f, 0.f, 4.f, 8.f);
    assert(ctrl.GetIntent().active);
    assert(ctrl.GetIntent().dwActState == 0u); // walk, no EM_ACT_RUN
    // yaw=0, axisX=0, axisY=1 => worldX = 0, worldZ = 1 (same convention as CharacterController)
    assert(near(ctrl.GetIntent().tarPos.x, 0.0f));
    assert(near(ctrl.GetIntent().tarPos.z, 4.0f)); // walkSpeed 4 * 1s

    std::cout << "[PASS] Joystick axis -> movement intent -> GOTO packet mapping\n";
}

static void test_touch_to_joystick_to_intent() {
    VirtualJoystick joy;
    joy.SetCenter(100.f, 100.f);
    joy.SetRadius(100.f);
    joy.SetDeadzone(0.1f);

    MovementInputController ctrl;
    // Touch down at center-right: should produce axisX~1
    assert(joy.TouchDown(1, 200.f, 100.f));
    joy.TouchMove(1, 200.f, 100.f);
    assert(joy.IsActive());
    assert(joy.GetMagnitude() > 0.9f);
    ctrl.Update(0.1f, joy.GetAxisX(), joy.GetAxisY(), joy.GetMagnitude(), 0.f,
                5.f, 0.f, 5.f, 4.f, 8.f);
    assert(ctrl.GetIntent().active);
    assert(ctrl.GetIntent().dwActState == EP7_EM_ACT_RUN);
    assert(ctrl.GetIntent().tarPos.x > 5.f); // moved +X at yaw 0

    // Touch up -> idle
    joy.TouchUp(1);
    assert(!joy.IsActive());
    ctrl.Update(0.1f, joy.GetAxisX(), joy.GetAxisY(), joy.GetMagnitude(), 0.f,
                ctrl.GetIntent().tarPos.x, 0.f, ctrl.GetIntent().tarPos.z, 4.f, 8.f);
    assert(!ctrl.GetIntent().active);
    assert(ctrl.GetStateMachine().GetState() == InputState::Idle);

    std::cout << "[PASS] Touch -> VirtualJoystick -> MovementIntent vertical slice\n";
}

static void test_camera_yaw_rotation_mapping() {
    MovementInputController ctrl;
    // yaw 90 degrees, axisX=1 axisY=0: worldX = 1*cos90 - 0*sin90 = 0, worldZ = 1*sin90 + 0*cos90 = 1
    ctrl.Update(1.0f, 1.0f, 0.0f, 1.0f, 90.0f, 0.f, 0.f, 0.f, 4.f, 8.f);
    assert(ctrl.GetIntent().active);
    assert(near(ctrl.GetIntent().tarPos.x, 0.0f, 1e-2f));
    assert(near(ctrl.GetIntent().tarPos.z, 8.0f, 1e-2f));
    std::cout << "[PASS] Camera yaw rotation mapping\n";
}

int main() {
    std::cout << "=== Phase 3.2 Input Binding Vertical Slice (offline) ===\n";
    test_state_machine_extensible();
    test_goto_packet_layout_evidence();
    test_joystick_to_movement_intent();
    test_touch_to_joystick_to_intent();
    test_camera_yaw_rotation_mapping();
    std::cout << "ALL INPUT BINDING TESTS PASSED!\n";
    return 0;
}
