#pragma once
// PR-A: Shader + Pipeline Bootstrap (GLES3). Runtime GL requires Android/GLES context;
// CPU-side math/validation is tested in sandbox. See report.

#include <cstdint>
#include <string>

namespace RANMobile {

struct ShaderBuildResult {
    bool ok = false;
    uint32_t program = 0;
    std::string log;
};

class ShaderPipeline {
public:
    // ENGINEERING_TEST_ASSET shaders — simple MVP+texture pipeline for validation only.
    static const char* TestVertexShaderSrc();
    static const char* TestFragmentShaderSrc();

    static ShaderBuildResult BuildProgram(const char* vsSrc, const char* fsSrc);
    static void DestroyProgram(uint32_t program);

    // Sets MVP (mat4 column-major float[16]) and sampler on currently bound program.
    static void SetMVP(uint32_t program, const float* mvp16);
    static void SetSampler(uint32_t program, int textureUnit);
};

} // namespace RANMobile
