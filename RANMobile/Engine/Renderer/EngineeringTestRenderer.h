#pragma once
// ENGINEERING_TEST_ASSET — not game content, used only to validate render pipeline mechanics.
// PR-B: static draw call with procedurally generated checkerboard texture and hardcoded cube mesh.

#include <cstdint>
#include <string>
#include "EngineeringTestAssets.h"
#include "RenderMath.h"

namespace RANMobile {

struct EngineeringTestRendererStats {
    bool initialized = false;
    bool runtimeGLAvailable = false;
    uint32_t drawCalls = 0;
    uint32_t program = 0;
    uint32_t vao = 0;
    uint32_t vbo = 0;
    uint32_t ibo = 0;
    uint32_t texture = 0;
    int indexCount = 0;
};

class EngineeringTestRenderer {
public:
    bool Initialize();
    void Shutdown();
    // Renders one test cube with MVP. No-op if runtime GL unavailable.
    void Render(float aspect, float timeSeconds);
    const EngineeringTestRendererStats& GetStats() const { return m_stats; }

    // CPU-side validation helpers (testable without GL context).
    static bool ValidateVertexLayout(); // pos3+uv2 stride/offsets
    static bool ValidateCubeUVRange();  // all UVs in [0,1]
    static bool ValidateCubeIndexRange();

private:
    EngineeringTestRendererStats m_stats;
};

} // namespace RANMobile
