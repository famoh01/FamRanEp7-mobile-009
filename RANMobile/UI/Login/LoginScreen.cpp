#include "LoginScreen.h"
#include "../../Client/Login/AuthenticationManager.h"
#include "../../Engine/Renderer/MobileRenderer.h"
#include <algorithm>
#include <cmath>

void LoginScreen::Initialize(RANMobile::NetworkClient* net) {
    m_networkClient = net; // Non-owning; engine owns the connection.
    m_serverDisplayName = RANMobile::AuthenticationManager::GetServerDisplayName();

    m_usernameField.SetTheme(&m_theme);
    m_usernameField.SetLabel("USERNAME");
    m_usernameField.SetPlaceholder("Enter username");
    m_usernameField.SetMaxLength(20);

    m_passwordField.SetTheme(&m_theme);
    m_passwordField.SetLabel("PASSWORD");
    m_passwordField.SetPlaceholder("Enter password");
    m_passwordField.SetPasswordMode(true);
    m_passwordField.SetMaxLength(20);

    m_loginButton.SetTheme(&m_theme);
    m_loginButton.SetLabel("LOGIN");
    m_loginButton.SetPrimary(true);
    m_loginButton.SetMinHeight(m_theme.loginButtonHeightDp);
    m_loginButton.SetOnClick([this]() { OnLoginPressed(); });

    m_registerButton.SetTheme(&m_theme);
    m_registerButton.SetLabel("REGISTER");
    m_registerButton.SetPrimary(false);

    m_forgotButton.SetTheme(&m_theme);
    m_forgotButton.SetLabel("FORGOT?");
    m_forgotButton.SetPrimary(false);

    m_settingsButton.SetTheme(&m_theme);
    m_settingsButton.SetLabel("SETTINGS");
    m_settingsButton.SetPrimary(false);

    m_exitButton.SetTheme(&m_theme);
    m_exitButton.SetLabel("EXIT");
    m_exitButton.SetPrimary(false);

    m_settingsCloseButton.SetTheme(&m_theme);
    m_settingsCloseButton.SetLabel("CLOSE");
    m_settingsCloseButton.SetPrimary(true);

    m_soundToggleButton.SetTheme(&m_theme);
    m_soundToggleButton.SetLabel("SOUND: ON");
    m_soundToggleButton.SetPrimary(false);
    m_soundToggleButton.SetOnClick([this]() {
        m_soundEnabled = !m_soundEnabled;
        m_soundToggleButton.SetLabel(m_soundEnabled ? "SOUND: ON" : "SOUND: OFF");
    });

    m_musicToggleButton.SetTheme(&m_theme);
    m_musicToggleButton.SetLabel("MUSIC: ON");
    m_musicToggleButton.SetPrimary(false);
    m_musicToggleButton.SetOnClick([this]() {
        m_musicEnabled = !m_musicEnabled;
        m_musicToggleButton.SetLabel(m_musicEnabled ? "MUSIC: ON" : "MUSIC: OFF");
    });

    m_exitYesButton.SetTheme(&m_theme);
    m_exitYesButton.SetLabel("YES");
    m_exitYesButton.SetPrimary(true);
    m_exitYesButton.SetOnClick([this]() { m_exitRequested = true; m_showExitConfirm = false; });

    m_exitNoButton.SetTheme(&m_theme);
    m_exitNoButton.SetLabel("NO");
    m_exitNoButton.SetPrimary(false);
    m_exitNoButton.SetOnClick([this]() { m_showExitConfirm = false; });

    m_settingsButton.SetOnClick([this]() { m_showSettingsPanel = true; m_showExitConfirm = false; });
    m_exitButton.SetOnClick([this]() { m_showExitConfirm = true; m_showSettingsPanel = false; });
    m_settingsCloseButton.SetOnClick([this]() { m_showSettingsPanel = false; });
    m_registerButton.SetOnClick([this]() { m_statusMessage = "REGISTER: PLEASE CONTACT SERVER ADMIN"; });
    m_forgotButton.SetOnClick([this]() { m_statusMessage = "PASSWORD RECOVERY: PLEASE CONTACT SERVER ADMIN"; });

    ComputeLayout(1280.0f, 720.0f);
}

void LoginScreen::SetUsername(const std::string& username) { m_usernameField.SetText(username); }
void LoginScreen::SetPassword(const std::string& password) { m_passwordField.SetText(password); }

LoginFocus LoginScreen::GetFocus() const {
    if (m_usernameField.IsFocused()) return LoginFocus::Username;
    if (m_passwordField.IsFocused()) return LoginFocus::Password;
    return LoginFocus::None;
}

void LoginScreen::SetScreenSize(float w, float h) {
    if (w <= 0 || h <= 0) return;
    if (std::abs(m_layout.screenW - w) > 0.5f || std::abs(m_layout.screenH - h) > 0.5f) {
        ComputeLayout(w, h);
    }
}

const LoginLayout& LoginScreen::ComputeLayout(float w, float h) {
    m_layout.screenW = w;
    m_layout.screenH = h;
    // Low-end fallback heuristic (auto): very small screens disable heavy effects.
    m_lowEndMode = (w < 720.0f || h < 480.0f);

    float panelW = std::min(460.0f, w - 48.0f);
    float panelH = std::min(560.0f, h - 48.0f);
    float px = (w - panelW) * 0.5f;
    float py = (h - panelH) * 0.5f;
    m_layout.panel = {px, py, panelW, panelH};

    float pad = 28.0f;
    float innerW = panelW - pad * 2.0f;
    float cursorY = py + 28.0f;
    m_layout.logoArea = {px + pad, cursorY, innerW, 72.0f};
    cursorY += 92.0f;

    m_layout.usernameField = {px + pad, cursorY, innerW, 56.0f};
    cursorY += 72.0f;
    m_layout.passwordField = {px + pad, cursorY, innerW, 56.0f};
    cursorY += 68.0f;

    m_layout.rememberToggle = {px + pad, cursorY, innerW * 0.5f, 32.0f};
    cursorY += 44.0f;

    m_layout.loginButton = {px + pad, cursorY, innerW, 56.0f};
    cursorY += 68.0f;

    float smallW = (innerW - 12.0f) * 0.5f;
    m_layout.registerButton = {px + pad, cursorY, smallW, 44.0f};
    m_layout.forgotButton = {px + pad + smallW + 12.0f, cursorY, smallW, 44.0f};
    cursorY += 56.0f;

    m_layout.settingsButton = {px + pad, cursorY, smallW, 44.0f};
    m_layout.exitButton = {px + pad + smallW + 12.0f, cursorY, smallW, 44.0f};

    // Settings modal (centered)
    float spW = std::min(420.0f, w - 64.0f);
    float spH = 260.0f;
    float spX = (w - spW) * 0.5f;
    float spY = (h - spH) * 0.5f;
    m_layout.settingsPanel = {spX, spY, spW, spH};
    m_layout.soundToggle = {spX + 24.0f, spY + 72.0f, spW - 48.0f, 48.0f};
    m_layout.musicToggle = {spX + 24.0f, spY + 128.0f, spW - 48.0f, 48.0f};
    m_layout.settingsClose = {spX + 24.0f, spY + spH - 64.0f, spW - 48.0f, 48.0f};

    // Exit confirm modal
    float ecW = std::min(360.0f, w - 64.0f);
    float ecH = 180.0f;
    float ecX = (w - ecW) * 0.5f;
    float ecY = (h - ecH) * 0.5f;
    m_layout.exitConfirmPanel = {ecX, ecY, ecW, ecH};
    float bw = (ecW - 56.0f) * 0.5f;
    m_layout.exitYes = {ecX + 20.0f, ecY + ecH - 64.0f, bw, 48.0f};
    m_layout.exitNo = {ecX + 36.0f + bw, ecY + ecH - 64.0f, bw, 48.0f};

    ApplyLayout();
    return m_layout;
}

void LoginScreen::ApplyLayout() {
    m_usernameField.SetRect(m_layout.usernameField.x, m_layout.usernameField.y, m_layout.usernameField.w, m_layout.usernameField.h);
    m_passwordField.SetRect(m_layout.passwordField.x, m_layout.passwordField.y, m_layout.passwordField.w, m_layout.passwordField.h);
    m_loginButton.SetRect(m_layout.loginButton.x, m_layout.loginButton.y, m_layout.loginButton.w, m_layout.loginButton.h);
    m_registerButton.SetRect(m_layout.registerButton.x, m_layout.registerButton.y, m_layout.registerButton.w, m_layout.registerButton.h);
    m_forgotButton.SetRect(m_layout.forgotButton.x, m_layout.forgotButton.y, m_layout.forgotButton.w, m_layout.forgotButton.h);
    m_settingsButton.SetRect(m_layout.settingsButton.x, m_layout.settingsButton.y, m_layout.settingsButton.w, m_layout.settingsButton.h);
    m_exitButton.SetRect(m_layout.exitButton.x, m_layout.exitButton.y, m_layout.exitButton.w, m_layout.exitButton.h);
    m_settingsCloseButton.SetRect(m_layout.settingsClose.x, m_layout.settingsClose.y, m_layout.settingsClose.w, m_layout.settingsClose.h);
    m_soundToggleButton.SetRect(m_layout.soundToggle.x, m_layout.soundToggle.y, m_layout.soundToggle.w, m_layout.soundToggle.h);
    m_musicToggleButton.SetRect(m_layout.musicToggle.x, m_layout.musicToggle.y, m_layout.musicToggle.w, m_layout.musicToggle.h);
    m_exitYesButton.SetRect(m_layout.exitYes.x, m_layout.exitYes.y, m_layout.exitYes.w, m_layout.exitYes.h);
    m_exitNoButton.SetRect(m_layout.exitNo.x, m_layout.exitNo.y, m_layout.exitNo.w, m_layout.exitNo.h);
}

void LoginScreen::HandleTouchDown(int pointerId, float x, float y) {
    // Modals first
    if (m_showExitConfirm) {
        if (m_exitNoButton.OnTouchDown(pointerId, x, y)) return;
        if (m_exitYesButton.OnTouchDown(pointerId, x, y)) return;
        // Tap outside modal: ignore (keep modal)
        return;
    }
    if (m_showSettingsPanel) {
        if (m_settingsCloseButton.OnTouchDown(pointerId, x, y)) return;
        if (m_soundToggleButton.OnTouchDown(pointerId, x, y)) return;
        if (m_musicToggleButton.OnTouchDown(pointerId, x, y)) return;
        return;
    }

    // Fields: focus management
    bool userHit = m_usernameField.OnTouchDown(pointerId, x, y);
    bool passHit = !userHit && m_passwordField.OnTouchDown(pointerId, x, y);
    if (userHit) { m_passwordField.SetFocused(false); return; }
    if (passHit) { m_usernameField.SetFocused(false); return; }

    // Buttons
    if (m_loginButton.OnTouchDown(pointerId, x, y)) { m_usernameField.SetFocused(false); m_passwordField.SetFocused(false); return; }
    if (m_registerButton.OnTouchDown(pointerId, x, y)) return;
    if (m_forgotButton.OnTouchDown(pointerId, x, y)) return;
    if (m_settingsButton.OnTouchDown(pointerId, x, y)) return;
    if (m_exitButton.OnTouchDown(pointerId, x, y)) return;

    // Tap elsewhere: clear focus
    m_usernameField.SetFocused(false);
    m_passwordField.SetFocused(false);
}

void LoginScreen::HandleTouchMove(int pointerId, float x, float y) {
    m_loginButton.OnTouchMove(pointerId, x, y);
    m_registerButton.OnTouchMove(pointerId, x, y);
    m_forgotButton.OnTouchMove(pointerId, x, y);
    m_settingsButton.OnTouchMove(pointerId, x, y);
    m_exitButton.OnTouchMove(pointerId, x, y);
    m_settingsCloseButton.OnTouchMove(pointerId, x, y);
    m_soundToggleButton.OnTouchMove(pointerId, x, y);
    m_musicToggleButton.OnTouchMove(pointerId, x, y);
    m_exitYesButton.OnTouchMove(pointerId, x, y);
    m_exitNoButton.OnTouchMove(pointerId, x, y);
}

void LoginScreen::HandleTouchUp(int pointerId, float x, float y) {
    m_loginButton.OnTouchUp(pointerId, x, y);
    m_registerButton.OnTouchUp(pointerId, x, y);
    m_forgotButton.OnTouchUp(pointerId, x, y);
    m_settingsButton.OnTouchUp(pointerId, x, y);
    m_exitButton.OnTouchUp(pointerId, x, y);
    m_settingsCloseButton.OnTouchUp(pointerId, x, y);
    m_soundToggleButton.OnTouchUp(pointerId, x, y);
    m_musicToggleButton.OnTouchUp(pointerId, x, y);
    m_exitYesButton.OnTouchUp(pointerId, x, y);
    m_exitNoButton.OnTouchUp(pointerId, x, y);
}

void LoginScreen::HandleTextInput(const std::string& utf8) {
    if (m_usernameField.IsFocused()) m_usernameField.OnTextInput(utf8);
    else if (m_passwordField.IsFocused()) m_passwordField.OnTextInput(utf8);
}

void LoginScreen::HandleBackspace() {
    if (m_usernameField.IsFocused()) m_usernameField.OnBackspace();
    else if (m_passwordField.IsFocused()) m_passwordField.OnBackspace();
}

void LoginScreen::Update(float deltaTime) {
    m_animTime += deltaTime;
    // Panel entrance: fade + scale (0.95->1.0), 280ms, ease-out
    float dur = m_theme.screenTransitionMs / 1000.0f;
    if (m_panelAnim < 1.0f) {
        m_panelAnim = std::min(1.0f, m_panelAnim + deltaTime / std::max(0.001f, dur));
    }

    m_usernameField.Update(deltaTime);
    m_passwordField.Update(deltaTime);
    m_loginButton.Update(deltaTime);
    m_registerButton.Update(deltaTime);
    m_forgotButton.Update(deltaTime);
    m_settingsButton.Update(deltaTime);
    m_exitButton.Update(deltaTime);
    m_settingsCloseButton.Update(deltaTime);
    m_soundToggleButton.Update(deltaTime);
    m_musicToggleButton.Update(deltaTime);
    m_exitYesButton.Update(deltaTime);
    m_exitNoButton.Update(deltaTime);
}

// Only authentication seam: do not reimplement packets/TEA in the UI.
// Existing Phase 3.4 manager owns opcode 2079, TEA and 56-byte login framing.
// true means sent/queued, NOT that server authentication succeeded.
bool LoginScreen::OnLoginPressed() {
    return RANMobile::AuthenticationManager::Login(m_networkClient,
                                                   m_usernameField.GetText(),
                                                   m_passwordField.GetText());
}

void LoginScreen::Draw() {}

void LoginScreen::DrawBackground(RANMobile::MobileRenderer* r) {
    // Full-bleed vertical gradient background
    r->DrawUIGradientRect(0, 0, m_layout.screenW, m_layout.screenH,
                          m_theme.bgTop.r, m_theme.bgTop.g, m_theme.bgTop.b, m_theme.bgTop.a,
                          m_theme.bgBottom.r, m_theme.bgBottom.g, m_theme.bgBottom.b, m_theme.bgBottom.a,
                          true);

    if (m_lowEndMode) return; // fallback: no particles/fog

    // Lightweight procedural fog/glow blobs (animated soft rounded rects)
    float t = m_animTime;
    auto blob = [&](float cx, float cy, float w, float h, float rC, float gC, float bC, float a, float phase) {
        float ox = std::sin(t * 0.25f + phase) * 18.0f;
        float oy = std::cos(t * 0.18f + phase * 1.3f) * 12.0f;
        r->DrawUIRoundedGlow(cx - w * 0.5f + ox, cy - h * 0.5f + oy, w, h, w * 0.35f, 40.0f, rC, gC, bC, a);
    };
    blob(m_layout.screenW * 0.25f, m_layout.screenH * 0.30f, 320.0f, 200.0f, 0.23f, 0.37f, 0.80f, 0.10f, 0.0f);
    blob(m_layout.screenW * 0.75f, m_layout.screenH * 0.65f, 280.0f, 180.0f, 0.83f, 0.69f, 0.22f, 0.08f, 1.7f);
    blob(m_layout.screenW * 0.50f, m_layout.screenH * 0.15f, 260.0f, 140.0f, 0.12f, 0.23f, 0.54f, 0.07f, 3.1f);
}

void LoginScreen::DrawPanel(RANMobile::MobileRenderer* r) {
    // Panel entrance animation: fade + scale 0.95->1.0
    float ease = 1.0f - std::pow(1.0f - m_panelAnim, 3.0f); // cubic-out
    float alpha = ease;
    float scale = 0.95f + 0.05f * ease;

    float pw = m_layout.panel.w * scale;
    float ph = m_layout.panel.h * scale;
    float px = m_layout.panel.x + (m_layout.panel.w - pw) * 0.5f;
    float py = m_layout.panel.y + (m_layout.panel.h - ph) * 0.5f;
    float radius = m_theme.cornerRadiusDp + 4.0f;

    // Shadow
    r->DrawUIRoundedShadow(px, py, pw, ph, radius, 0.0f, 6.0f, 18.0f, 0.0f, 0.0f, 0.0f, 0.28f * alpha);

    // Glass fill (rgba 255,255,255,0.06). True backdrop blur not available in current renderer;
    // fallback: layered translucent fill gives glass look. blur target = 12px (theme token).
    r->DrawUIRoundedRect(px, py, pw, ph, radius,
                         m_theme.glassFill.r, m_theme.glassFill.g, m_theme.glassFill.b, m_theme.glassFill.a * alpha);
    r->DrawUIRoundedRect(px, py, pw, ph * 0.45f, radius,
                         1.0f, 1.0f, 1.0f, 0.025f * alpha);

    // Border glow
    r->DrawUIRoundedGlow(px, py, pw, ph, radius, m_lowEndMode ? 0.0f : m_theme.borderGlowMinPx,
                         m_theme.borderGlow.r, m_theme.borderGlow.g, m_theme.borderGlow.b, m_theme.borderGlow.a * alpha);
    r->DrawUIRoundedBorder(px, py, pw, ph, radius, m_theme.borderWidthPx,
                           m_theme.borderGlow.r, m_theme.borderGlow.g, m_theme.borderGlow.b, 0.55f * alpha);

    // Logo / title
    // PLACEHOLDER_LOGO — awaiting official asset (no logo file found in repo/ThaiPack)
    float titleScale = 1.15f; // ~32sp visual at base font
    float tw = r->MeasureTextWidth(m_gameTitle, titleScale);
    float tx = px + (pw - tw) * 0.5f;
    float ty = py + 28.0f;
    r->DrawUIText(tx, ty, m_gameTitle, titleScale, m_theme.goldEnd.r, m_theme.goldEnd.g, m_theme.goldEnd.b, alpha);

    float subScale = 0.72f;
    std::string sub = m_serverDisplayName;
    float sw = r->MeasureTextWidth(sub, subScale);
    r->DrawUIText(px + (pw - sw) * 0.5f, ty + 34.0f, sub, subScale, m_theme.textSecondary.r, m_theme.textSecondary.g, m_theme.textSecondary.b, 0.9f * alpha);

    // Divider line under logo
    r->DrawUIQuad(px + 40.0f, ty + 58.0f, pw - 80.0f, 1.0f, m_theme.goldStart.r, m_theme.goldStart.g, m_theme.goldStart.b, 0.25f * alpha);

    // Status message
    if (!m_statusMessage.empty()) {
        float ss = 0.52f;
        float stw = r->MeasureTextWidth(m_statusMessage, ss);
        RANMobile::UIColor sc = m_theme.textSecondary;
        if (m_authState == LoginAuthState::LoginRejected || m_authState == LoginAuthState::ConnectionError) sc = m_theme.errorColor;
        if (m_authState == LoginAuthState::LoginAccepted) sc = m_theme.successColor;
        r->DrawUIText(px + (pw - stw) * 0.5f, py + ph - 22.0f, m_statusMessage, ss, sc.r, sc.g, sc.b, 0.9f * alpha);
    }

    // Components
    m_usernameField.Draw(r);
    m_passwordField.Draw(r);
    m_loginButton.Draw(r);
    m_registerButton.Draw(r);
    m_forgotButton.Draw(r);
    m_settingsButton.Draw(r);
    m_exitButton.Draw(r);
}

void LoginScreen::DrawSettingsModal(RANMobile::MobileRenderer* r) {
    // Dim background
    r->DrawUIQuad(0, 0, m_layout.screenW, m_layout.screenH, 0.0f, 0.0f, 0.0f, 0.55f);
    const auto& p = m_layout.settingsPanel;
    r->DrawUIRoundedShadow(p.x, p.y, p.w, p.h, 14.0f, 0, 4, 16, 0, 0, 0, 0.35f);
    r->DrawUIRoundedRect(p.x, p.y, p.w, p.h, 14.0f, 0.10f, 0.12f, 0.18f, 0.96f);
    r->DrawUIRoundedBorder(p.x, p.y, p.w, p.h, 14.0f, 1.5f, m_theme.borderGlow.r, m_theme.borderGlow.g, m_theme.borderGlow.b, 0.45f);
    r->DrawUIText(p.x + 24.0f, p.y + 24.0f, "SETTINGS", 0.85f, m_theme.goldEnd.r, m_theme.goldEnd.g, m_theme.goldEnd.b, 1.0f);
    m_soundToggleButton.Draw(r);
    m_musicToggleButton.Draw(r);
    m_settingsCloseButton.Draw(r);
}

void LoginScreen::DrawExitConfirmModal(RANMobile::MobileRenderer* r) {
    r->DrawUIQuad(0, 0, m_layout.screenW, m_layout.screenH, 0.0f, 0.0f, 0.0f, 0.55f);
    const auto& p = m_layout.exitConfirmPanel;
    r->DrawUIRoundedShadow(p.x, p.y, p.w, p.h, 14.0f, 0, 4, 16, 0, 0, 0, 0.35f);
    r->DrawUIRoundedRect(p.x, p.y, p.w, p.h, 14.0f, 0.10f, 0.12f, 0.18f, 0.96f);
    r->DrawUIRoundedBorder(p.x, p.y, p.w, p.h, 14.0f, 1.5f, m_theme.borderGlow.r, m_theme.borderGlow.g, m_theme.borderGlow.b, 0.45f);
    r->DrawUIText(p.x + 24.0f, p.y + 28.0f, "EXIT GAME?", 0.85f, m_theme.textPrimary.r, m_theme.textPrimary.g, m_theme.textPrimary.b, 1.0f);
    r->DrawUIText(p.x + 24.0f, p.y + 58.0f, "Are you sure you want to exit?", 0.55f, m_theme.textSecondary.r, m_theme.textSecondary.g, m_theme.textSecondary.b, 1.0f);
    m_exitYesButton.Draw(r);
    m_exitNoButton.Draw(r);
}

void LoginScreen::Draw(RANMobile::MobileRenderer* renderer) {
    if (!renderer) return;
    SetScreenSize(static_cast<float>(renderer->GetWidth()), static_cast<float>(renderer->GetHeight()));
    DrawBackground(renderer);
    DrawPanel(renderer);
    if (m_showSettingsPanel) DrawSettingsModal(renderer);
    if (m_showExitConfirm) DrawExitConfirmModal(renderer);
}
