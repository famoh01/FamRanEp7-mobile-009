#pragma once

#include "GraphicsSettings.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace RANMobile {

struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t triangles = 0;
    uint32_t culledObjects = 0;
    uint32_t instancedBatches = 0;
};

struct GlyphInfo {
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    float w = 0.0f, h = 0.0f;
    float xoff = 0.0f, yoff = 0.0f;
    float advance = 0.0f;
    bool valid = false;
};

class MobileRenderer {
public:
    bool Initialize(int width, int height);
    void Resize(int width, int height);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void DrawUIQuad(float x, float y, float width, float height, float r, float g, float b, float a);
    void DrawUIGradientRect(float x, float y, float width, float height,
                            float r1, float g1, float b1, float a1,
                            float r2, float g2, float b2, float a2,
                            bool vertical = true);
    void DrawUIRoundedRect(float x, float y, float w, float h, float radius,
                           float r, float g, float b, float a);
    void DrawUIGradientRoundedRect(float x, float y, float w, float h, float radius,
                                   float r1, float g1, float b1, float a1,
                                   float r2, float g2, float b2, float a2,
                                   bool vertical = true);
    void DrawUIRoundedBorder(float x, float y, float w, float h, float radius,
                             float borderWidth,
                             float r, float g, float b, float a);
    void DrawUIRoundedGlow(float x, float y, float w, float h, float radius,
                           float glowSize, float r, float g, float b, float a);
    void DrawUIRoundedShadow(float x, float y, float w, float h, float radius,
                             float offsetX, float offsetY, float blur,
                             float r, float g, float b, float a);
    void DrawUIText(float x, float y, const std::string& text, float scale, float r, float g, float b, float a);
    float MeasureTextWidth(const std::string& text, float scale);

    uint32_t UploadRGBA8Texture(uint32_t width, uint32_t height, const uint8_t* rgba8);
    void DeleteTexture(uint32_t texId);

    static uint32_t DecodeUTF8(const std::string& text, size_t& index);

    void ApplySettings(const GraphicsSettings& settings);
    const GraphicsSettings& GetSettings() const { return m_settings; }
    const RenderStats& GetStats() const { return m_stats; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    bool IsInitialized() const { return m_initialized; }

    bool IsBoxInFrustum(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) const;

private:
    const GlyphInfo* GetOrLoadGlyph(uint32_t codepoint);
    bool LoadFontFile(const std::string& path, std::vector<uint8_t>& outBuffer);

    bool m_initialized = false;
    int m_width = 1280;
    int m_height = 720;
    GraphicsSettings m_settings;
    RenderStats m_stats;

    std::vector<uint8_t> m_primaryFontData;
    std::vector<uint8_t> m_thaiFontData;
    bool m_hasPrimaryFont = false;
    bool m_hasThaiFont = false;
    std::unordered_map<uint32_t, GlyphInfo> m_glyphCache;
    int m_atlasX = 1;
    int m_atlasY = 1;
    int m_atlasRowH = 0;

#ifdef __ANDROID__
    uint32_t m_uiProgram = 0;
    uint32_t m_uiVAO = 0;
    uint32_t m_uiVBO = 0;
    uint32_t m_fontAtlasTex = 0;
    int32_t m_uProjLoc = -1;
    int32_t m_uColorLoc = -1;
    int32_t m_uUseTexLoc = -1;

    // Phase 5 extended UI rect shader (rounded/gradient/border/glow/shadow)
    uint32_t m_uiRectProgram = 0;
    int32_t m_urProjLoc = -1;
    int32_t m_urRectLoc = -1;
    int32_t m_urRadiusLoc = -1;
    int32_t m_urColorTopLoc = -1;
    int32_t m_urColorBottomLoc = -1;
    int32_t m_urUseGradientLoc = -1;
    int32_t m_urBorderWidthLoc = -1;
    int32_t m_urBorderColorLoc = -1;
    int32_t m_urGlowSizeLoc = -1;
    int32_t m_urGlowColorLoc = -1;
    int32_t m_urShadowOffsetLoc = -1;
    int32_t m_urShadowBlurLoc = -1;
    int32_t m_urModeLoc = -1; // 0=fill,1=border,2=glow,3=shadow
#endif
};

} // namespace RANMobile
