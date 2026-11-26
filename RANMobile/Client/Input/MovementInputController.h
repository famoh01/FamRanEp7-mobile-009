#pragma once

#include "InputStateMachine.h"
#include "../../Engine/Network/EP7Protocol.h"
#include <cstdint>

namespace RANMobile {

struct MovementIntent {
    bool active = false;
    InputIntentType type = InputIntentType::None;
    uint32_t dwActState = 0;
    EP7Vec3 curPos{};
    EP7Vec3 tarPos{};
};

class MovementInputController {
public:
    void Reset();
    void Update(float deltaTime,
                float axisX, float axisY, float magnitude,
                float cameraYawDeg,
                float curPosX, float curPosY, float curPosZ,
                float walkSpeed, float runSpeed);

    const MovementIntent& GetIntent() const { return m_intent; }
    InputStateMachine& GetStateMachine() { return m_sm; }
    const InputStateMachine& GetStateMachine() const { return m_sm; }

    // Build source-evidenced SNETPC_GOTO packet from current intent.
    // Returns false if intent is not active.
    bool BuildGotoPacket(EP7SNetPCGoto& out) const;

private:
    InputStateMachine m_sm;
    MovementIntent m_intent;
};

} // namespace RANMobile
