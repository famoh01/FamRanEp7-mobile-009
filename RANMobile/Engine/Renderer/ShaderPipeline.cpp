#include "ShaderPipeline.h"
#include "../Core/Logger.h"

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#endif

namespace RANMobile {

const char* ShaderPipeline::TestVertexShaderSrc() {
    // ENGINEERING_TEST_ASSET — not game content, used only to validate render pipeline mechanics.
    return R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aUV;
uniform mat4 uMVP;
out vec2 vUV;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vUV = aUV;
}
)";
}

const char* ShaderPipeline::TestFragmentShaderSrc() {
    // ENGINEERING_TEST_ASSET — not game content, used only to validate render pipeline mechanics.
    return R"(#version 300 es
precision mediump float;
in vec2 vUV;
uniform sampler2D uTex;
out vec4 FragColor;
void main() {
    FragColor = texture(uTex, vUV);
}
)";
}

ShaderBuildResult ShaderPipeline::BuildProgram(const char* vsSrc, const char* fsSrc) {
    ShaderBuildResult r;
#ifdef __ANDROID__
    auto compile = [&](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512] = {};
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            r.log += log;
            glDeleteShader(s);
            return 0;
        }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        r.ok = false;
        return r;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        r.log += log;
        glDeleteProgram(p);
        r.ok = false;
        return r;
    }
    r.ok = true;
    r.program = static_cast<uint32_t>(p);
    return r;
#else
    (void)vsSrc; (void)fsSrc;
    r.ok = false;
    r.log = "GLES runtime not available in this build (compile-time verified only)";
    return r;
#endif
}

void ShaderPipeline::DestroyProgram(uint32_t program) {
#ifdef __ANDROID__
    if (program) glDeleteProgram(static_cast<GLuint>(program));
#else
    (void)program;
#endif
}

void ShaderPipeline::SetMVP(uint32_t program, const float* mvp16) {
#ifdef __ANDROID__
    GLint loc = glGetUniformLocation(static_cast<GLuint>(program), "uMVP");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, mvp16);
#else
    (void)program; (void)mvp16;
#endif
}

void ShaderPipeline::SetSampler(uint32_t program, int textureUnit) {
#ifdef __ANDROID__
    GLint loc = glGetUniformLocation(static_cast<GLuint>(program), "uTex");
    if (loc >= 0) glUniform1i(loc, textureUnit);
#else
    (void)program; (void)textureUnit;
#endif
}

} // namespace RANMobile
