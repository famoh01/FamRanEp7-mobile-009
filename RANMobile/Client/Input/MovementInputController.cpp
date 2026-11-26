#include "MovementInputController.h"
#include <cmath>
#include <algorithm>

namespace RANMobile {

void MovementInputController::Reset() {
    m_sm.Reset();
    m_intent = MovementIntent{};
}

void MovementInputController::Update(float deltaTime,
                                     float axisX, float axisY, float magnitude,
                                     float cameraYawDeg,
                                     float curPosX, float curPosY, float curPosZ,
                                     float walkSpeed, float runSpeed) {
    m_intent = MovementIntent{};
    m_intent.curPos = {curPosX, curPosY, curPosZ};
    m_intent.tarPos = {curPosX, curPosY, curPosZ};

    if (magnitude < 0.05f) {
        m_sm.OnIntent(InputIntentType::None);
        m_intent.active = false;
        m_intent.type = InputIntentType::None;
        m_intent.dwActState = 0;
        return;
    }

    const bool running = magnitude > 0.65f;
    const float speed = running ? runSpeed : walkSpeed;
    const float yawRad = cameraYawDeg * 3.14159265f / 180.0f;
    const float cosYaw = std::cos(yawRad);
    const float sinYaw = std::sin(yawRad);
    // Same axis convention as CharacterController: joystick Y maps to forward.
    const float worldX = axisX * cosYaw - axisY * sinYaw;
    const float worldZ = axisX * sinYaw + axisY * cosYaw;

    m_intent.active = true;
    m_intent.type = InputIntentType::Movement;
    m_intent.dwActState = running ? EP7_EM_ACT_RUN : 0u;
    m_intent.tarPos.x = curPosX + worldX * speed * std::max(deltaTime, 0.0f);
    m_intent.tarPos.y = curPosY;
    m_intent.tarPos.z = curPosZ + worldZ * speed * std::max(deltaTime, 0.0f);
    m_sm.OnIntent(InputIntentType::Movement);
}

bool MovementInputController::BuildGotoPacket(EP7SNetPCGoto& out) const {
    if (!m_intent.active) return false;
    out = EP7SNetPCGoto{}; // sets dwSize/nType
    out.dwActState = m_intent.dwActState;
    out.vCurPos = m_intent.curPos;
    out.vTarPos = m_intent.tarPos;
    return true;
}

} // namespace RANMobile
