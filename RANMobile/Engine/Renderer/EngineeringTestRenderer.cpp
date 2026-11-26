// ENGINEERING_TEST_ASSET — not game content, used only to validate render pipeline mechanics.
#include "EngineeringTestRenderer.h"
#include "ShaderPipeline.h"
#include <cstddef>
#include "../Core/Logger.h"

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#endif

namespace RANMobile {

bool EngineeringTestRenderer::ValidateVertexLayout() {
    // GLES3 spec: attribute 0 vec3 at offset 0, attribute 1 vec2 at offset 12, stride 20.
    static_assert(sizeof(EngineeringTestAssets::VertexPosUV) == 20, "stride");
    static_assert(offsetof(EngineeringTestAssets::VertexPosUV, x) == 0, "pos offset");
    static_assert(offsetof(EngineeringTestAssets::VertexPosUV, u) == 12, "uv offset");
    return sizeof(EngineeringTestAssets::VertexPosUV) == 20 &&
           offsetof(EngineeringTestAssets::VertexPosUV, x) == 0 &&
           offsetof(EngineeringTestAssets::VertexPosUV, u) == 12;
}

bool EngineeringTestRenderer::ValidateCubeUVRange() {
    auto mesh = EngineeringTestAssets::MakeUnitCube();
    for (const auto& v : mesh.vertices) {
        if (v.u < 0.0f || v.u > 1.0f || v.v < 0.0f || v.v > 1.0f) return false;
    }
    return !mesh.vertices.empty();
}

bool EngineeringTestRenderer::ValidateCubeIndexRange() {
    auto mesh = EngineeringTestAssets::MakeUnitCube();
    if (mesh.indices.empty() || mesh.vertices.empty()) return false;
    for (uint16_t idx : mesh.indices) {
        if (idx >= mesh.vertices.size()) return false;
    }
    return mesh.indices.size() % 3 == 0;
}

bool EngineeringTestRenderer::Initialize() {
    Shutdown();
    m_stats = EngineeringTestRendererStats{};
    auto mesh = EngineeringTestAssets::MakeUnitCube();
    m_stats.indexCount = static_cast<int>(mesh.indices.size());
#ifdef __ANDROID__
    auto prog = ShaderPipeline::BuildProgram(ShaderPipeline::TestVertexShaderSrc(),
                                             ShaderPipeline::TestFragmentShaderSrc());
    if (!prog.ok) {
        Logger::Error("EngineeringTestRenderer", "Shader build failed: " + prog.log);
        return false;
    }
    m_stats.program = prog.program;
    glGenVertexArrays(1, reinterpret_cast<GLuint*>(&m_stats.vao));
    glGenBuffers(1, reinterpret_cast<GLuint*>(&m_stats.vbo));
    glGenBuffers(1, reinterpret_cast<GLuint*>(&m_stats.ibo));
    glBindVertexArray(static_cast<GLuint>(m_stats.vao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(m_stats.vbo));
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(EngineeringTestAssets::VertexPosUV),
                 mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(m_stats.ibo));
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint16_t),
                 mesh.indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(EngineeringTestAssets::VertexPosUV),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(EngineeringTestAssets::VertexPosUV),
                          reinterpret_cast<void*>(12));
    glBindVertexArray(0);

    auto tex = EngineeringTestAssets::MakeCheckerboardRGBA8(64,64,8, 200,200,200,255, 40,40,40,255);
    glGenTextures(1, reinterpret_cast<GLuint*>(&m_stats.texture));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_stats.texture));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    m_stats.runtimeGLAvailable = true;
    m_stats.initialized = true;
    Logger::Info("EngineeringTestRenderer", "Initialized (ENGINEERING_TEST_ASSET cube+checkerboard)");
    return true;
#else
    // No GLES runtime in sandbox build; CPU-side validation still possible.
    m_stats.runtimeGLAvailable = false;
    m_stats.initialized = true;
    Logger::Info("EngineeringTestRenderer", "Stub init (no GLES runtime, compile-time verified only)");
    return true;
#endif
}

void EngineeringTestRenderer::Render(float aspect, float timeSeconds) {
#ifdef __ANDROID__
    if (!m_stats.initialized || !m_stats.runtimeGLAvailable || !m_stats.program) return;
    glUseProgram(static_cast<GLuint>(m_stats.program));
    Mat4 proj = Mat4Perspective(60.0f * 3.14159265f / 180.0f, aspect > 0.1f ? aspect : 1.0f, 0.1f, 100.0f);
    Mat4 view = Mat4LookAt({0,1.5f,3.0f}, {0,0,0}, {0,1,0});
    Mat4 model = Mat4Multiply(Mat4Translate(0,0,0), Mat4RotateY(timeSeconds * 0.8f));
    Mat4 mvp = Mat4Multiply(proj, Mat4Multiply(view, model));
    ShaderPipeline::SetMVP(m_stats.program, mvp.m.data());
    ShaderPipeline::SetSampler(m_stats.program, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_stats.texture));
    glBindVertexArray(static_cast<GLuint>(m_stats.vao));
    glDrawElements(GL_TRIANGLES, m_stats.indexCount, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
    m_stats.drawCalls++;
#else
    (void)aspect; (void)timeSeconds;
#endif
}

void EngineeringTestRenderer::Shutdown() {
#ifdef __ANDROID__
    if (m_stats.texture) glDeleteTextures(1, reinterpret_cast<GLuint*>(&m_stats.texture));
    if (m_stats.ibo) glDeleteBuffers(1, reinterpret_cast<GLuint*>(&m_stats.ibo));
    if (m_stats.vbo) glDeleteBuffers(1, reinterpret_cast<GLuint*>(&m_stats.vbo));
    if (m_stats.vao) glDeleteVertexArrays(1, reinterpret_cast<GLuint*>(&m_stats.vao));
    if (m_stats.program) ShaderPipeline::DestroyProgram(m_stats.program);
#endif
    m_stats = EngineeringTestRendererStats{};
}

} // namespace RANMobile
