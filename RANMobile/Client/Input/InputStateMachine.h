#pragma once
#include <cstdint>

namespace RANMobile {

// Extensible input intent types for the mobile client.
// Phase 3.2 implements Movement only; Skill/Inventory are placeholders for
// future phases (state machine must be extensible per master command).
enum class InputIntentType : uint8_t {
    None = 0,
    Movement = 1,
    Skill = 2,      // future: Phase 3.x / UI
    Inventory = 3,  // future
    Interact = 4    // future
};

enum class InputState : uint8_t {
    Idle = 0,
    Moving = 1,
    // Future states can be added without breaking Movement flow.
    CastingSkill = 2,
    InventoryOpen = 3
};

// Minimal extensible state machine. Movement is the only active transition
// implemented in Phase 3.2.
class InputStateMachine {
public:
    InputState GetState() const { return m_state; }

    void OnIntent(InputIntentType intent) {
        switch (intent) {
            case InputIntentType::Movement:
                m_state = InputState::Moving;
                break;
            case InputIntentType::Skill:
                // Placeholder: accept transition to keep extensibility contract.
                m_state = InputState::CastingSkill;
                break;
            case InputIntentType::Inventory:
                m_state = InputState::InventoryOpen;
                break;
            case InputIntentType::None:
            case InputIntentType::Interact:
            default:
                m_state = InputState::Idle;
                break;
        }
    }

    void Reset() { m_state = InputState::Idle; }

private:
    InputState m_state = InputState::Idle;
};

} // namespace RANMobile
