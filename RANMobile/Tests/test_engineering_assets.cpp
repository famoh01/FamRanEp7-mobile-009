// PR-B: Static Draw Call with Test Texture - CPU-side validation.
// ENGINEERING_TEST_ASSET — not game content, used only to validate render pipeline mechanics.
// Validates UV mapping, vertex layout (GLES3 spec), index ranges, and procedural texture generation.
// Runtime GL upload/draw is Android/device-verified (B2).

#include "../Engine/Renderer/EngineeringTestAssets.h"
#include "../Engine/Renderer/EngineeringTestRenderer.h"
#include <cassert>
#include <cstddef>
#include <iostream>

using namespace RANMobile;
using namespace RANMobile::EngineeringTestAssets;

static void test_checkerboard() {
    auto tex = MakeCheckerboardRGBA8(64,64,8, 255,0,0,255, 0,255,0,255);
    assert(tex.size() == 64*64*4);
    // (0,0) should be color1
    assert(tex[0]==255 && tex[1]==0 && tex[2]==0 && tex[3]==255);
    // (8,0) should be color2
    size_t i = (0*64 + 8) * 4;
    assert(tex[i]==0 && tex[i+1]==255 && tex[i+2]==0 && tex[i+3]==255);
    std::cout << "[PASS] Checkerboard RGBA8 generation (ENGINEERING_TEST_ASSET)\n";
}

static void test_gradient() {
    auto tex = MakeGradientRGBA8(4,1, 0,0,0, 255,255,255);
    assert(tex.size() == 4*1*4);
    assert(tex[0]==0 && tex[3]==255);
    assert(tex[12]==255 && tex[13]==255 && tex[14]==255);
    std::cout << "[PASS] Gradient RGBA8 generation (ENGINEERING_TEST_ASSET)\n";
}

static void test_quad_layout() {
    auto q = MakeUnitQuad();
    assert(q.vertices.size() == 4);
    assert(q.indices.size() == 6);
    // GLES3 layout: location 0 vec3 at offset 0, location 1 vec2 at offset 12, stride 20
    assert(sizeof(VertexPosUV) == 20);
    assert(offsetof(VertexPosUV, x) == 0);
    assert(offsetof(VertexPosUV, u) == 12);
    for (auto idx : q.indices) assert(idx < q.vertices.size());
    for (const auto& v : q.vertices) {
        assert(v.u >= 0.f && v.u <= 1.f && v.v >= 0.f && v.v <= 1.f);
    }
    std::cout << "[PASS] UnitQuad vertex layout/UV/index (GLES3 spec)\n";
}

static void test_cube_layout() {
    auto c = MakeUnitCube();
    assert(c.vertices.size() == 24); // 6 faces * 4 verts
    assert(c.indices.size() == 36);  // 6 faces * 2 tri * 3
    assert(EngineeringTestRenderer::ValidateVertexLayout());
    assert(EngineeringTestRenderer::ValidateCubeUVRange());
    assert(EngineeringTestRenderer::ValidateCubeIndexRange());
    for (const auto& v : c.vertices) {
        assert(v.x >= -0.5f && v.x <= 0.5f);
        assert(v.y >= -0.5f && v.y <= 0.5f);
        assert(v.z >= -0.5f && v.z <= 0.5f);
    }
    std::cout << "[PASS] UnitCube vertex layout/UV/index/range (ENGINEERING_TEST_ASSET)\n";
}

int main() {
    std::cout << "=== PR-B Engineering Test Assets (CPU validation) ===\n";
    test_checkerboard();
    test_gradient();
    test_quad_layout();
    test_cube_layout();
    std::cout << "ALL ENGINEERING ASSET TESTS PASSED!\n";
    return 0;
}
