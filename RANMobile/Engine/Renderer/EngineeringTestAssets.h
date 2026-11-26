#pragma once
// ENGINEERING_TEST_ASSET — not game content, used only to validate render pipeline mechanics.
// All geometry/textures here are procedurally generated debug fixtures (checkerboard,
// solid gradient, unit cube/quad). Do NOT treat as RAN Online game assets.

#include <cstdint>
#include <vector>

namespace RANMobile {
namespace EngineeringTestAssets {

struct VertexPosUV {
    float x, y, z;
    float u, v;
};
static_assert(sizeof(VertexPosUV) == 20, "Pos3+UV2 = 20 bytes");

struct MeshData {
    std::vector<VertexPosUV> vertices;
    std::vector<uint16_t> indices;
};

// Generates an RGBA8 checkerboard texture. Deterministic, no external files.
// ENGINEERING_TEST_ASSET — not game content.
std::vector<uint8_t> MakeCheckerboardRGBA8(uint32_t width, uint32_t height, uint32_t cellSize,
                                           uint8_t r1,uint8_t g1,uint8_t b1,uint8_t a1,
                                           uint8_t r2,uint8_t g2,uint8_t b2,uint8_t a2);

// Generates a simple horizontal RGBA8 gradient. ENGINEERING_TEST_ASSET.
std::vector<uint8_t> MakeGradientRGBA8(uint32_t width, uint32_t height,
                                       uint8_t r1,uint8_t g1,uint8_t b1,
                                       uint8_t r2,uint8_t g2,uint8_t b2);

// Unit cube centered at origin, size 1.0, with UVs. 24 vertices (4 per face) for clean UVs.
// ENGINEERING_TEST_ASSET — not game content.
MeshData MakeUnitCube();

// Unit quad on XY plane centered at origin, facing +Z, size 1.0.
// ENGINEERING_TEST_ASSET — not game content.
MeshData MakeUnitQuad();

} // namespace EngineeringTestAssets
} // namespace RANMobile
