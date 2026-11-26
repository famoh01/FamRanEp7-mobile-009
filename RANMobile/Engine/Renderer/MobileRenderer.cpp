#include "MobileRenderer.h"
#include "../Core/Logger.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <fstream>
#include <cstring>

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#endif

namespace RANMobile {

static constexpr int kAtlasSize = 1024;
static constexpr float kBaseFontPx = 28.0f;

static stbtt_fontinfo g_primaryFontInfo;
static stbtt_fontinfo g_thaiFontInfo;

#ifdef __ANDROID__
static const char* kUIVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
uniform mat4 uProj;
out vec2 vUV;
void main() {
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

static const char* kUIFragmentShader = R"(#version 300 es
precision mediump float;
uniform vec4 uColor;
uniform int uUseTexture;
uniform sampler2D uTex;
in vec2 vUV;
out vec4 FragColor;
void main() {
    if (uUseTexture == 1) {
        float alpha = texture(uTex, vUV).r;
        FragColor = vec4(uColor.rgb, uColor.a * alpha);
    } else if (uUseTexture == 2) {
        FragColor = texture(uTex, vUV) * uColor;
    } else {
        FragColor = uColor;
    }
}
)";

// Phase 5 extended rect shader: rounded SDF fill/gradient/border/glow/shadow.
// Used for modern UI panels/buttons; keeps existing simple UI shader intact.
static const char* kUIRectVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
uniform mat4 uProj;
out vec2 vPos;
out vec2 vUV;
void main() {
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
    vPos = aPos;
    vUV = aUV;
}
)";

static const char* kUIRectFragmentShader = R"(#version 300 es
precision mediump float;
in vec2 vPos;
in vec2 vUV;
uniform vec4 uRect;        // x,y,w,h in screen px (top-left origin)
uniform float uRadius;
uniform vec4 uColorTop;
uniform vec4 uColorBottom;
uniform int uUseGradient;  // 0=solid(top),1=vertical,2=horizontal
uniform float uBorderWidth;
uniform vec4 uBorderColor;
uniform float uGlowSize;
uniform vec4 uGlowColor;
uniform vec2 uShadowOffset;
uniform float uShadowBlur;
uniform int uMode;         // 0=fill,1=border,2=glow,3=shadow
out vec4 FragColor;

float sdRoundRect(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + vec2(r);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    vec2 center = uRect.xy + uRect.zw * 0.5;
    vec2 halfSize = uRect.zw * 0.5;
    float r = clamp(uRadius, 0.0, min(halfSize.x, halfSize.y));
    vec2 p = vPos - center;
    float d = sdRoundRect(p, halfSize, r);

    vec4 base = uColorTop;
    if (uUseGradient == 1) {
        float t = clamp((vPos.y - uRect.y) / max(uRect.w, 1.0), 0.0, 1.0);
        base = mix(uColorTop, uColorBottom, t);
    } else if (uUseGradient == 2) {
        float t = clamp((vPos.x - uRect.x) / max(uRect.z, 1.0), 0.0, 1.0);
        base = mix(uColorTop, uColorBottom, t);
    }

    float aa = 1.0;
    if (uMode == 0) {
        float fillA = 1.0 - smoothstep(-aa, aa, d);
        FragColor = vec4(base.rgb, base.a * fillA);
    } else if (uMode == 1) {
        float bw = max(uBorderWidth, 0.0);
        float outer = 1.0 - smoothstep(-aa, aa, d);
        float inner = 1.0 - smoothstep(-aa, aa, d + bw);
        float a = clamp(outer - inner, 0.0, 1.0);
        FragColor = vec4(uBorderColor.rgb, uBorderColor.a * a);
    } else if (uMode == 2) {
        float gs = max(uGlowSize, 0.5);
        // Glow outside the rounded rect boundary.
        float glow = exp(-max(d, 0.0) / gs) * (1.0 - smoothstep(-aa, aa, d));
        FragColor = vec4(uGlowColor.rgb, uGlowColor.a * glow);
    } else if (uMode == 3) {
        vec2 sp = vPos - center - uShadowOffset;
        float sd = sdRoundRect(sp, halfSize, r);
        float sb = max(uShadowBlur, 1.0);
        float shadow = 1.0 - smoothstep(0.0, sb, sd);
        FragColor = vec4(base.rgb, base.a * shadow);
    } else {
        FragColor = vec4(0.0);
    }
    if (FragColor.a <= 0.001) discard;
}
)";

static GLuint CompileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        Logger::Error("MobileRenderer", std::string("Shader compile error: ") + log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
#endif

uint32_t MobileRenderer::DecodeUTF8(const std::string& text, size_t& index) {
    if (index >= text.size()) return 0;
    uint8_t c = static_cast<uint8_t>(text[index]);
    if (c < 0x80) {
        index += 1;
        return c;
    } else if ((c & 0xE0) == 0xC0 && index + 1 < text.size()) {
        uint32_t cp = ((c & 0x1F) << 6) | (static_cast<uint8_t>(text[index + 1]) & 0x3F);
        index += 2;
        return cp;
    } else if ((c & 0xF0) == 0xE0 && index + 2 < text.size()) {
        uint32_t cp = ((c & 0x0F) << 12) |
                      ((static_cast<uint8_t>(text[index + 1]) & 0x3F) << 6) |
                      (static_cast<uint8_t>(text[index + 2]) & 0x3F);
        index += 3;
        return cp;
    } else if ((c & 0xF8) == 0xF0 && index + 3 < text.size()) {
        uint32_t cp = ((c & 0x07) << 18) |
                      ((static_cast<uint8_t>(text[index + 1]) & 0x3F) << 12) |
                      ((static_cast<uint8_t>(text[index + 2]) & 0x3F) << 6) |
                      (static_cast<uint8_t>(text[index + 3]) & 0x3F);
        index += 4;
        return cp;
    }
    index += 1;
    return 0xFFFD;
}

bool MobileRenderer::LoadFontFile(const std::string& path, std::vector<uint8_t>& outBuffer) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return false;
    std::streamsize sz = f.tellg();
    if (sz <= 0) return false;
    f.seekg(0, std::ios::beg);
    outBuffer.resize(static_cast<size_t>(sz));
    return f.read(reinterpret_cast<char*>(outBuffer.data()), sz).good();
}

bool MobileRenderer::Initialize(int width, int height) {
    m_width = width > 0 ? width : 1280;
    m_height = height > 0 ? height : 720;
    m_settings.ApplyPreset(GraphicsQuality::Medium);

    const char* primaryPaths[] = {
        "/system/fonts/Roboto-Regular.ttf",
        "/system/fonts/NotoSans-Regular.ttf",
        "/system/fonts/DroidSans.ttf"
    };
    for (const char* p : primaryPaths) {
        if (LoadFontFile(p, m_primaryFontData) &&
            stbtt_InitFont(&g_primaryFontInfo, m_primaryFontData.data(), 0)) {
            m_hasPrimaryFont = true;
            Logger::Info("MobileRenderer", std::string("Loaded primary font: ") + p);
            break;
        }
    }

    const char* thaiPaths[] = {
        "/system/fonts/NotoSansThai-Regular.ttf",
        "/system/fonts/NotoSansThaiUI-Regular.ttf",
        "/system/fonts/DroidSansFallback.ttf"
    };
    for (const char* p : thaiPaths) {
        if (LoadFontFile(p, m_thaiFontData) &&
            stbtt_InitFont(&g_thaiFontInfo, m_thaiFontData.data(), 0)) {
            m_hasThaiFont = true;
            Logger::Info("MobileRenderer", std::string("Loaded Thai font: ") + p);
            break;
        }
    }

#ifdef __ANDROID__
    glViewport(0, 0, m_width, m_height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.05f, 0.07f, 0.12f, 1.0f);

    GLuint vs = CompileShader(GL_VERTEX_SHADER, kUIVertexShader);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kUIFragmentShader);
    if (vs && fs) {
        m_uiProgram = glCreateProgram();
        glAttachShader(m_uiProgram, vs);
        glAttachShader(m_uiProgram, fs);
        glLinkProgram(m_uiProgram);
        glDeleteShader(vs);
        glDeleteShader(fs);
        m_uProjLoc = glGetUniformLocation(m_uiProgram, "uProj");
        m_uColorLoc = glGetUniformLocation(m_uiProgram, "uColor");
        m_uUseTexLoc = glGetUniformLocation(m_uiProgram, "uUseTexture");
    }

    // Phase 5 extended rect shader
    {
        GLuint rvs = CompileShader(GL_VERTEX_SHADER, kUIRectVertexShader);
        GLuint rfs = CompileShader(GL_FRAGMENT_SHADER, kUIRectFragmentShader);
        if (rvs && rfs) {
            m_uiRectProgram = glCreateProgram();
            glAttachShader(m_uiRectProgram, rvs);
            glAttachShader(m_uiRectProgram, rfs);
            glLinkProgram(m_uiRectProgram);
            glDeleteShader(rvs);
            glDeleteShader(rfs);
            m_urProjLoc = glGetUniformLocation(m_uiRectProgram, "uProj");
            m_urRectLoc = glGetUniformLocation(m_uiRectProgram, "uRect");
            m_urRadiusLoc = glGetUniformLocation(m_uiRectProgram, "uRadius");
            m_urColorTopLoc = glGetUniformLocation(m_uiRectProgram, "uColorTop");
            m_urColorBottomLoc = glGetUniformLocation(m_uiRectProgram, "uColorBottom");
            m_urUseGradientLoc = glGetUniformLocation(m_uiRectProgram, "uUseGradient");
            m_urBorderWidthLoc = glGetUniformLocation(m_uiRectProgram, "uBorderWidth");
            m_urBorderColorLoc = glGetUniformLocation(m_uiRectProgram, "uBorderColor");
            m_urGlowSizeLoc = glGetUniformLocation(m_uiRectProgram, "uGlowSize");
            m_urGlowColorLoc = glGetUniformLocation(m_uiRectProgram, "uGlowColor");
            m_urShadowOffsetLoc = glGetUniformLocation(m_uiRectProgram, "uShadowOffset");
            m_urShadowBlurLoc = glGetUniformLocation(m_uiRectProgram, "uShadowBlur");
            m_urModeLoc = glGetUniformLocation(m_uiRectProgram, "uMode");
        } else {
            if (rvs) glDeleteShader(rvs);
            if (rfs) glDeleteShader(rfs);
        }
    }

    glGenVertexArrays(1, &m_uiVAO);
    glGenBuffers(1, &m_uiVBO);
    glBindVertexArray(m_uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);

    glGenTextures(1, &m_fontAtlasTex);
    glBindTexture(GL_TEXTURE_2D, m_fontAtlasTex);
    std::vector<uint8_t> zeros(kAtlasSize * kAtlasSize, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kAtlasSize, kAtlasSize, 0, GL_RED, GL_UNSIGNED_BYTE, zeros.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    m_initialized = true;
    Logger::Info("MobileRenderer", "Initialized OpenGL ES 3.0 Renderer (" +
                 std::to_string(m_width) + "x" + std::to_string(m_height) + ")");
    return true;
}

void MobileRenderer::Resize(int width, int height) {
    m_width = width > 0 ? width : 1280;
    m_height = height > 0 ? height : 720;
#ifdef __ANDROID__
    if (m_initialized) {
        glViewport(0, 0, m_width, m_height);
    }
#endif
}

void MobileRenderer::Shutdown() {
#ifdef __ANDROID__
    if (m_fontAtlasTex) { glDeleteTextures(1, &m_fontAtlasTex); m_fontAtlasTex = 0; }
    if (m_uiVBO) { glDeleteBuffers(1, &m_uiVBO); m_uiVBO = 0; }
    if (m_uiVAO) { glDeleteVertexArrays(1, &m_uiVAO); m_uiVAO = 0; }
    if (m_uiRectProgram) { glDeleteProgram(m_uiRectProgram); m_uiRectProgram = 0; }
    if (m_uiProgram) { glDeleteProgram(m_uiProgram); m_uiProgram = 0; }
#endif
    m_glyphCache.clear();
    m_initialized = false;
}

void MobileRenderer::BeginFrame() {
    m_stats = RenderStats{};
#ifdef __ANDROID__
    if (m_initialized) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
#endif
}

void MobileRenderer::EndFrame() {
}

const GlyphInfo* MobileRenderer::GetOrLoadGlyph(uint32_t codepoint) {
    auto it = m_glyphCache.find(codepoint);
    if (it != m_glyphCache.end()) return &it->second;

    GlyphInfo info;
    stbtt_fontinfo* font = nullptr;
    if (codepoint >= 0x0E00 && codepoint <= 0x0E7F && m_hasThaiFont) {
        font = &g_thaiFontInfo;
    } else if (m_hasPrimaryFont && stbtt_FindGlyphIndex(&g_primaryFontInfo, static_cast<int>(codepoint)) != 0) {
        font = &g_primaryFontInfo;
    } else if (m_hasThaiFont && stbtt_FindGlyphIndex(&g_thaiFontInfo, static_cast<int>(codepoint)) != 0) {
        font = &g_thaiFontInfo;
    }

    if (!font) {
        info.advance = kBaseFontPx * 0.5f;
        info.valid = false;
        m_glyphCache[codepoint] = info;
        return &m_glyphCache[codepoint];
    }

    float scale = stbtt_ScaleForPixelHeight(font, kBaseFontPx);
    int adv = 0, lsb = 0;
    stbtt_GetCodepointHMetrics(font, static_cast<int>(codepoint), &adv, &lsb);
    info.advance = adv * scale;

    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(font, static_cast<int>(codepoint), scale, scale, &x0, &y0, &x1, &y1);
    int gw = x1 - x0;
    int gh = y1 - y0;
    info.w = static_cast<float>(gw);
    info.h = static_cast<float>(gh);
    info.xoff = static_cast<float>(x0);
    info.yoff = static_cast<float>(y0) + kBaseFontPx * 0.8f;

    if (gw > 0 && gh > 0 && m_atlasY + gh + 2 < kAtlasSize) {
        if (m_atlasX + gw + 2 >= kAtlasSize) {
            m_atlasX = 1;
            m_atlasY += m_atlasRowH + 2;
            m_atlasRowH = 0;
        }
        if (m_atlasY + gh + 2 < kAtlasSize) {
#ifdef __ANDROID__
            std::vector<uint8_t> bmp(static_cast<size_t>(gw * gh), 0);
            stbtt_MakeCodepointBitmap(font, bmp.data(), gw, gh, gw, scale, scale, static_cast<int>(codepoint));
            glBindTexture(GL_TEXTURE_2D, m_fontAtlasTex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, m_atlasX, m_atlasY, gw, gh, GL_RED, GL_UNSIGNED_BYTE, bmp.data());
            glBindTexture(GL_TEXTURE_2D, 0);
#endif
            info.u0 = static_cast<float>(m_atlasX) / kAtlasSize;
            info.v0 = static_cast<float>(m_atlasY) / kAtlasSize;
            info.u1 = static_cast<float>(m_atlasX + gw) / kAtlasSize;
            info.v1 = static_cast<float>(m_atlasY + gh) / kAtlasSize;
            m_atlasX += gw + 2;
            if (gh > m_atlasRowH) m_atlasRowH = gh;
            info.valid = true;
        }
    }

    m_glyphCache[codepoint] = info;
    return &m_glyphCache[codepoint];
}

void MobileRenderer::DrawUIQuad(float x, float y, float width, float height, float r, float g, float b, float a) {
    m_stats.drawCalls++;
    m_stats.triangles += 2;
#ifdef __ANDROID__
    if (!m_initialized || !m_uiProgram) return;
    glUseProgram(m_uiProgram);
    float L = 0.0f, R = static_cast<float>(m_width);
    float T = 0.0f, B = static_cast<float>(m_height);
    float proj[16] = {
        2.0f / (R - L), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (T - B), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -(R + L) / (R - L), -(T + B) / (T - B), 0.0f, 1.0f
    };
    glUniformMatrix4fv(m_uProjLoc, 1, GL_FALSE, proj);
    glUniform4f(m_uColorLoc, r, g, b, a);
    glUniform1i(m_uUseTexLoc, 0);

    float verts[16] = {
        x, y, 0.0f, 0.0f,
        x + width, y, 1.0f, 0.0f,
        x, y + height, 0.0f, 1.0f,
        x + width, y + height, 1.0f, 1.0f
    };
    glBindVertexArray(m_uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
#else
    (void)x; (void)y; (void)width; (void)height; (void)r; (void)g; (void)b; (void)a;
#endif
}

#ifdef __ANDROID__
static void DrawUIRectInternal(
    uint32_t program,
    int32_t uProj, int32_t uRect, int32_t uRadius,
    int32_t uColorTop, int32_t uColorBottom, int32_t uUseGradient,
    int32_t uBorderWidth, int32_t uBorderColor,
    int32_t uGlowSize, int32_t uGlowColor,
    int32_t uShadowOffset, int32_t uShadowBlur, int32_t uMode,
    uint32_t vao, uint32_t vbo,
    int screenW, int screenH,
    float x, float y, float w, float h, float radius,
    float r1, float g1, float b1, float a1,
    float r2, float g2, float b2, float a2,
    int gradientMode,
    float borderWidth, float br, float bg, float bb, float ba,
    float glowSize, float gr, float gg, float gb, float ga,
    float shOx, float shOy, float shBlur,
    int mode)
{
    if (!program) return;
    glUseProgram(program);
    float L = 0.0f, Rr = static_cast<float>(screenW);
    float T = 0.0f, B = static_cast<float>(screenH);
    float proj[16] = {
        2.0f / (Rr - L), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (T - B), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -(Rr + L) / (Rr - L), -(T + B) / (T - B), 0.0f, 1.0f
    };
    glUniformMatrix4fv(uProj, 1, GL_FALSE, proj);
    glUniform4f(uRect, x, y, w, h);
    glUniform1f(uRadius, radius);
    glUniform4f(uColorTop, r1, g1, b1, a1);
    glUniform4f(uColorBottom, r2, g2, b2, a2);
    glUniform1i(uUseGradient, gradientMode);
    glUniform1f(uBorderWidth, borderWidth);
    glUniform4f(uBorderColor, br, bg, bb, ba);
    glUniform1f(uGlowSize, glowSize);
    glUniform4f(uGlowColor, gr, gg, gb, ga);
    glUniform2f(uShadowOffset, shOx, shOy);
    glUniform1f(uShadowBlur, shBlur);
    glUniform1i(uMode, mode);

    // Expand quad for glow/shadow to avoid clipping the effect.
    float pad = 0.0f;
    if (mode == 2) pad = glowSize * 3.0f;
    if (mode == 3) pad = shBlur * 2.0f + std::max(std::abs(shOx), std::abs(shOy));
    float qx = x - pad, qy = y - pad, qw = w + pad * 2.0f, qh = h + pad * 2.0f;
    float verts[16] = {
        qx, qy, 0.0f, 0.0f,
        qx + qw, qy, 1.0f, 0.0f,
        qx, qy + qh, 0.0f, 1.0f,
        qx + qw, qy + qh, 1.0f, 1.0f
    };
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
#endif

void MobileRenderer::DrawUIGradientRect(float x, float y, float width, float height,
                                        float r1, float g1, float b1, float a1,
                                        float r2, float g2, float b2, float a2,
                                        bool vertical) {
    m_stats.drawCalls++;
    m_stats.triangles += 2;
#ifdef __ANDROID__
    DrawUIRectInternal(m_uiRectProgram,
        m_urProjLoc, m_urRectLoc, m_urRadiusLoc, m_urColorTopLoc, m_urColorBottomLoc, m_urUseGradientLoc,
        m_urBorderWidthLoc, m_urBorderColorLoc, m_urGlowSizeLoc, m_urGlowColorLoc,
        m_urShadowOffsetLoc, m_urShadowBlurLoc, m_urModeLoc,
        m_uiVAO, m_uiVBO, m_width, m_height,
        x, y, width, height, 0.0f,
        r1, g1, b1, a1, r2, g2, b2, a2, vertical ? 1 : 2,
        0,0,0,0,0, 0,0,0,0,0, 0,0,0, 0);
#else
    (void)x; (void)y; (void)width; (void)height;
    (void)r1; (void)g1; (void)b1; (void)a1; (void)r2; (void)g2; (void)b2; (void)a2; (void)vertical;
#endif
}

void MobileRenderer::DrawUIRoundedRect(float x, float y, float w, float h, float radius,
                                       float r, float g, float b, float a) {
    m_stats.drawCalls++;
    m_stats.triangles += 2;
#ifdef __ANDROID__
    DrawUIRectInternal(m_uiRectProgram,
        m_urProjLoc, m_urRectLoc, m_urRadiusLoc, m_urColorTopLoc, m_urColorBottomLoc, m_urUseGradientLoc,
        m_urBorderWidthLoc, m_urBorderColorLoc, m_urGlowSizeLoc, m_urGlowColorLoc,
        m_urShadowOffsetLoc, m_urShadowBlurLoc, m_urModeLoc,
        m_uiVAO, m_uiVBO, m_width, m_height,
        x, y, w, h, radius,
        r, g, b, a, r, g, b, a, 0,
        0,0,0,0,0, 0,0,0,0,0, 0,0,0, 0);
#else
    (void)x; (void)y; (void)w; (void)h; (void)radius; (void)r; (void)g; (void)b; (void)a;
#endif
}

void MobileRenderer::DrawUIGradientRoundedRect(float x, float y, float w, float h, float radius,
                                               float r1, float g1, float b1, float a1,
                                               float r2, float g2, float b2, float a2,
                                               bool vertical) {
    m_stats.drawCalls++;
    m_stats.triangles += 2;
#ifdef __ANDROID__
    DrawUIRectInternal(m_uiRectProgram,
        m_urProjLoc, m_urRectLoc, m_urRadiusLoc, m_urColorTopLoc, m_urColorBottomLoc, m_urUseGradientLoc,
        m_urBorderWidthLoc, m_urBorderColorLoc, m_urGlowSizeLoc, m_urGlowColorLoc,
        m_urShadowOffsetLoc, m_urShadowBlurLoc, m_urModeLoc,
        m_uiVAO, m_uiVBO, m_width, m_height,
        x, y, w, h, radius,
        r1, g1, b1, a1, r2, g2, b2, a2, vertical ? 1 : 2,
        0,0,0,0,0, 0,0,0,0,0, 0,0,0, 0);
#else
    (void)x; (void)y; (void)w; (void)h; (void)radius;
    (void)r1; (void)g1; (void)b1; (void)a1; (void)r2; (void)g2; (void)b2; (void)a2; (void)vertical;
#endif
}

void MobileRenderer::DrawUIRoundedBorder(float x, float y, float w, float h, float radius,
                                         float borderWidth,
                                         float r, float g, float b, float a) {
    m_stats.drawCalls++;
    m_stats.triangles += 2;
#ifdef __ANDROID__
    DrawUIRectInternal(m_uiRectProgram,
        m_urProjLoc, m_urRectLoc, m_urRadiusLoc, m_urColorTopLoc, m_urColorBottomLoc, m_urUseGradientLoc,
        m_urBorderWidthLoc, m_urBorderColorLoc, m_urGlowSizeLoc, m_urGlowColorLoc,
        m_urShadowOffsetLoc, m_urShadowBlurLoc, m_urModeLoc,
        m_uiVAO, m_uiVBO, m_width, m_height,
        x, y, w, h, radius,
        0,0,0,0, 0,0,0,0, 0,
        borderWidth, r,g,b,a, 0,0,0,0,0, 0,0,0, 1);
#else
    (void)x; (void)y; (void)w; (void)h; (void)radius; (void)borderWidth; (void)r; (void)g; (void)b; (void)a;
#endif
}

void MobileRenderer::DrawUIRoundedGlow(float x, float y, float w, float h, float radius,
                                       float glowSize, float r, float g, float b, float a) {
    m_stats.drawCalls++;
    m_stats.triangles += 2;
#ifdef __ANDROID__
    DrawUIRectInternal(m_uiRectProgram,
        m_urProjLoc, m_urRectLoc, m_urRadiusLoc, m_urColorTopLoc, m_urColorBottomLoc, m_urUseGradientLoc,
        m_urBorderWidthLoc, m_urBorderColorLoc, m_urGlowSizeLoc, m_urGlowColorLoc,
        m_urShadowOffsetLoc, m_urShadowBlurLoc, m_urModeLoc,
        m_uiVAO, m_uiVBO, m_width, m_height,
        x, y, w, h, radius,
        0,0,0,0, 0,0,0,0, 0,
        0,0,0,0,0, glowSize, r,g,b,a, 0,0,0, 2);
#else
    (void)x; (void)y; (void)w; (void)h; (void)radius; (void)glowSize; (void)r; (void)g; (void)b; (void)a;
#endif
}

void MobileRenderer::DrawUIRoundedShadow(float x, float y, float w, float h, float radius,
                                         float offsetX, float offsetY, float blur,
                                         float r, float g, float b, float a) {
    m_stats.drawCalls++;
    m_stats.triangles += 2;
#ifdef __ANDROID__
    DrawUIRectInternal(m_uiRectProgram,
        m_urProjLoc, m_urRectLoc, m_urRadiusLoc, m_urColorTopLoc, m_urColorBottomLoc, m_urUseGradientLoc,
        m_urBorderWidthLoc, m_urBorderColorLoc, m_urGlowSizeLoc, m_urGlowColorLoc,
        m_urShadowOffsetLoc, m_urShadowBlurLoc, m_urModeLoc,
        m_uiVAO, m_uiVBO, m_width, m_height,
        x, y, w, h, radius,
        r,g,b,a, r,g,b,a, 0,
        0,0,0,0,0, 0,0,0,0,0, offsetX, offsetY, blur, 3);
#else
    (void)x; (void)y; (void)w; (void)h; (void)radius; (void)offsetX; (void)offsetY; (void)blur;
    (void)r; (void)g; (void)b; (void)a;
#endif
}

void MobileRenderer::DrawUIText(float x, float y, const std::string& text, float scale, float r, float g, float b, float a) {
    if (text.empty()) return;
    float s = scale * 0.85f;
    float cursorX = x;
    size_t idx = 0;
    while (idx < text.size()) {
        uint32_t cp = DecodeUTF8(text, idx);
        const GlyphInfo* gInfo = GetOrLoadGlyph(cp);
        if (!gInfo) continue;

        if (gInfo->valid && gInfo->w > 0 && gInfo->h > 0) {
            m_stats.drawCalls++;
            m_stats.triangles += 2;
#ifdef __ANDROID__
            if (m_initialized && m_uiProgram && m_fontAtlasTex) {
                glUseProgram(m_uiProgram);
                float L = 0.0f, R = static_cast<float>(m_width);
                float T = 0.0f, B = static_cast<float>(m_height);
                float proj[16] = {
                    2.0f / (R - L), 0.0f, 0.0f, 0.0f,
                    0.0f, 2.0f / (T - B), 0.0f, 0.0f,
                    0.0f, 0.0f, -1.0f, 0.0f,
                    -(R + L) / (R - L), -(T + B) / (T - B), 0.0f, 1.0f
                };
                glUniformMatrix4fv(m_uProjLoc, 1, GL_FALSE, proj);
                glUniform4f(m_uColorLoc, r, g, b, a);
                glUniform1i(m_uUseTexLoc, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_fontAtlasTex);

                float gx = cursorX + gInfo->xoff * s;
                float gy = y + gInfo->yoff * s;
                float gw = gInfo->w * s;
                float gh = gInfo->h * s;
                float verts[16] = {
                    gx, gy, gInfo->u0, gInfo->v0,
                    gx + gw, gy, gInfo->u1, gInfo->v0,
                    gx, gy + gh, gInfo->u0, gInfo->v1,
                    gx + gw, gy + gh, gInfo->u1, gInfo->v1
                };
                glBindVertexArray(m_uiVAO);
                glBindBuffer(GL_ARRAY_BUFFER, m_uiVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
#else
            (void)y; (void)r; (void)g; (void)b; (void)a;
#endif
        }
        cursorX += gInfo->advance * s;
    }
}

float MobileRenderer::MeasureTextWidth(const std::string& text, float scale) {
    float s = scale * 0.85f;
    float w = 0.0f;
    size_t idx = 0;
    while (idx < text.size()) {
        uint32_t cp = DecodeUTF8(text, idx);
        const GlyphInfo* gInfo = GetOrLoadGlyph(cp);
        if (gInfo) w += gInfo->advance * s;
    }
    return w;
}

void MobileRenderer::ApplySettings(const GraphicsSettings& settings) {
    m_settings = settings;
}

bool MobileRenderer::IsBoxInFrustum(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) const {
    (void)minY;
    (void)maxY;
    float centerX = (minX + maxX) * 0.5f;
    float centerZ = (minZ + maxZ) * 0.5f;
    float distSq = centerX * centerX + centerZ * centerZ;
    return distSq <= (m_settings.viewDistance * m_settings.viewDistance);
}

uint32_t MobileRenderer::UploadRGBA8Texture(uint32_t width, uint32_t height, const uint8_t* rgba8) {
    if (!m_initialized || width == 0 || height == 0 || !rgba8) return 0;
#ifdef __ANDROID__
    GLuint texId = 0;
    glGenTextures(1, &texId);
    if (texId == 0) return 0;
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba8);
    glBindTexture(GL_TEXTURE_2D, 0);
    return static_cast<uint32_t>(texId);
#else
    return 1; // Non-Android headless test stub
#endif
}

void MobileRenderer::DeleteTexture(uint32_t texId) {
    if (texId == 0) return;
#ifdef __ANDROID__
    GLuint id = static_cast<GLuint>(texId);
    glDeleteTextures(1, &id);
#endif
}

} // namespace RANMobile
