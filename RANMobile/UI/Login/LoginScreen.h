#pragma once

#include <string>
#include <vector>
#include "../UITheme.h"
#include "../Components/UIButton.h"
#include "../Components/UIInputField.h"

namespace RANMobile {
    class NetworkClient;
    class MobileRenderer;
}

enum class LoginFocus {
    None,
    Username,
    Password
};

enum class LoginAuthState {
    Disconnected,
    Connecting,
    Connected,
    Authenticating,
    LoginAccepted,
    LoginRejected,
    ConnectionError
};

struct LoginLayout {
    float screenW = 1280.0f;
    float screenH = 720.0f;
    RANMobile::UIRect panel;
    RANMobile::UIRect logoArea;
    RANMobile::UIRect usernameField;
    RANMobile::UIRect passwordField;
    RANMobile::UIRect rememberToggle;
    RANMobile::UIRect loginButton;
    RANMobile::UIRect registerButton;
    RANMobile::UIRect forgotButton;
    RANMobile::UIRect settingsButton;
    RANMobile::UIRect exitButton;
    // Settings modal
    RANMobile::UIRect settingsPanel;
    RANMobile::UIRect soundToggle;
    RANMobile::UIRect musicToggle;
    RANMobile::UIRect settingsClose;
    // Exit confirm modal
    RANMobile::UIRect exitConfirmPanel;
    RANMobile::UIRect exitYes;
    RANMobile::UIRect exitNo;
};

// Phase 5: Modern Fantasy Luxury Login Screen (procedural/vector UI).
// Auth seam preserved: OnLoginPressed() delegates to AuthenticationManager only.
// No auth packet/opcode/TEA/ABI changes.
class LoginScreen {
public:
    void Initialize(RANMobile::NetworkClient* net);

    void SetUsername(const std::string& username);
    void SetPassword(const std::string& password);
    void SetRememberAccount(bool remember) { m_rememberAccount = remember; }
    void ToggleShowPassword() { m_showPassword = !m_showPassword; }
    bool IsPasswordVisible() const { return m_showPassword; }

    const std::string& GetGameTitle() const { return m_gameTitle; }
    const std::string& GetServerDisplayName() const { return m_serverDisplayName; }
    const std::string& GetUsername() const { return m_usernameField.GetText(); }
    std::string GetMaskedPassword() const { return std::string(m_passwordField.GetText().size(), '*'); }
    bool GetRememberAccount() const { return m_rememberAccount; }
    LoginFocus GetFocus() const;
    LoginAuthState GetAuthState() const { return m_authState; }
    const std::string& GetStatusMessage() const { return m_statusMessage; }
    bool WantsExit() const { return m_exitRequested; }
    bool IsSettingsOpen() const { return m_showSettingsPanel; }
    bool IsExitConfirmOpen() const { return m_showExitConfirm; }
    bool IsSoundEnabled() const { return m_soundEnabled; }
    bool IsMusicEnabled() const { return m_musicEnabled; }
    int GetGraphicsLevel() const { return m_graphicsLevel; }

    void HandleTouchDown(int pointerId, float x, float y);
    void HandleTouchMove(int pointerId, float x, float y);
    void HandleTouchUp(int pointerId, float x, float y);
    void HandleTextInput(const std::string& utf8);
    void HandleBackspace();
    void Update(float deltaTime);

    bool OnLoginPressed();
    void Draw();
    void Draw(RANMobile::MobileRenderer* renderer);

    // Layout helpers for tests / deterministic hit-testing
    void SetScreenSize(float w, float h);
    const LoginLayout& GetLayout() const { return m_layout; }
    const LoginLayout& ComputeLayout(float w, float h);

private:
    void ApplyLayout();
    void SyncLegacyFields();
    void DrawBackground(RANMobile::MobileRenderer* r);
    void DrawPanel(RANMobile::MobileRenderer* r);
    void DrawSettingsModal(RANMobile::MobileRenderer* r);
    void DrawExitConfirmModal(RANMobile::MobileRenderer* r);

    RANMobile::NetworkClient* m_networkClient = nullptr;
    RANMobile::UITheme m_theme;
    LoginLayout m_layout;

    std::string m_gameTitle = "RAN ONLINE EP7";
    std::string m_serverDisplayName = "FAM HAWAII";

    RANMobile::UIInputField m_usernameField;
    RANMobile::UIInputField m_passwordField;
    RANMobile::UIButton m_loginButton;
    RANMobile::UIButton m_registerButton;
    RANMobile::UIButton m_forgotButton;
    RANMobile::UIButton m_settingsButton;
    RANMobile::UIButton m_exitButton;
    RANMobile::UIButton m_settingsCloseButton;
    RANMobile::UIButton m_soundToggleButton;
    RANMobile::UIButton m_musicToggleButton;
    RANMobile::UIButton m_exitYesButton;
    RANMobile::UIButton m_exitNoButton;

    bool m_rememberAccount = false;
    bool m_showPassword = false;
    bool m_showSettingsPanel = false;
    bool m_showExitConfirm = false;
    bool m_soundEnabled = true;
    bool m_musicEnabled = true;
    int m_graphicsLevel = 1; // 0=LOW, 1=MED, 2=HIGH, 3=ULTRA

    LoginAuthState m_authState = LoginAuthState::Disconnected;
    std::string m_statusMessage = "DISCONNECTED";
    bool m_exitRequested = false;

    float m_animTime = 0.0f;
    float m_panelAnim = 0.0f; // 0..1 fade+scale in
    bool m_lowEndMode = false; // fallback: disable heavy blur/glow/particles
};
