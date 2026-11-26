// ENGINEERING_TEST_ASSET — not game content, used only to validate render pipeline mechanics.
#include "EngineeringTestAssets.h"
#include <algorithm>

namespace RANMobile {
namespace EngineeringTestAssets {

std::vector<uint8_t> MakeCheckerboardRGBA8(uint32_t width, uint32_t height, uint32_t cellSize,
                                           uint8_t r1,uint8_t g1,uint8_t b1,uint8_t a1,
                                           uint8_t r2,uint8_t g2,uint8_t b2,uint8_t a2) {
    if (cellSize == 0) cellSize = 1;
    std::vector<uint8_t> out(static_cast<size_t>(width) * height * 4, 0);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const bool alt = ((x / cellSize) + (y / cellSize)) % 2 == 0;
            const size_t i = (static_cast<size_t>(y) * width + x) * 4;
            out[i+0] = alt ? r1 : r2;
            out[i+1] = alt ? g1 : g2;
            out[i+2] = alt ? b1 : b2;
            out[i+3] = alt ? a1 : a2;
        }
    }
    return out;
}

std::vector<uint8_t> MakeGradientRGBA8(uint32_t width, uint32_t height,
                                       uint8_t r1,uint8_t g1,uint8_t b1,
                                       uint8_t r2,uint8_t g2,uint8_t b2) {
    std::vector<uint8_t> out(static_cast<size_t>(width) * height * 4, 255);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const float t = (width <= 1) ? 0.0f : static_cast<float>(x) / static_cast<float>(width - 1);
            const size_t i = (static_cast<size_t>(y) * width + x) * 4;
            out[i+0] = static_cast<uint8_t>(r1 + (r2 - r1) * t);
            out[i+1] = static_cast<uint8_t>(g1 + (g2 - g1) * t);
            out[i+2] = static_cast<uint8_t>(b1 + (b2 - b1) * t);
            out[i+3] = 255;
        }
    }
    return out;
}

MeshData MakeUnitQuad() {
    // ENGINEERING_TEST_ASSET — simple quad, not game content.
    MeshData m;
    m.vertices = {
        {-0.5f,-0.5f,0.0f, 0.0f,1.0f},
        { 0.5f,-0.5f,0.0f, 1.0f,1.0f},
        { 0.5f, 0.5f,0.0f, 1.0f,0.0f},
        {-0.5f, 0.5f,0.0f, 0.0f,0.0f},
    };
    m.indices = {0,1,2, 0,2,3};
    return m;
}

MeshData MakeUnitCube() {
    // ENGINEERING_TEST_ASSET — unit cube, not game content.
    MeshData m;
    const float h = 0.5f;
    // Each face: 4 vertices, UV 0..1
    struct F { float nx,ny,nz; float v[4][5]; };
    // Vertices: x,y,z,u,v
    auto addFace = [&](float ax,float ay,float az, float bx,float by,float bz,
                       float cx,float cy,float cz, float dx,float dy,float dz) {
        uint16_t base = static_cast<uint16_t>(m.vertices.size());
        m.vertices.push_back({ax,ay,az,0,1});
        m.vertices.push_back({bx,by,bz,1,1});
        m.vertices.push_back({cx,cy,cz,1,0});
        m.vertices.push_back({dx,dy,dz,0,0});
        m.indices.insert(m.indices.end(), {base,uint16_t(base+1),uint16_t(base+2), base,uint16_t(base+2),uint16_t(base+3)});
    };
    // Front (+Z)
    addFace(-h,-h, h,  h,-h, h,  h, h, h, -h, h, h);
    // Back (-Z)
    addFace( h,-h,-h, -h,-h,-h, -h, h,-h,  h, h,-h);
    // Left (-X)
    addFace(-h,-h,-h, -h,-h, h, -h, h, h, -h, h,-h);
    // Right (+X)
    addFace( h,-h, h,  h,-h,-h,  h, h,-h,  h, h, h);
    // Top (+Y)
    addFace(-h, h, h,  h, h, h,  h, h,-h, -h, h,-h);
    // Bottom (-Y)
    addFace(-h,-h,-h,  h,-h,-h,  h,-h, h, -h,-h, h);
    return m;
}

} // namespace EngineeringTestAssets
} // namespace RANMobile
