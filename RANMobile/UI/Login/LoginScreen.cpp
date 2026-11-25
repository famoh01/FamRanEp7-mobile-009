#include "LoginScreen.h"
#include "../../Client/Login/AuthenticationManager.h"

void LoginScreen::Initialize(RANMobile::NetworkClient* net) {
    m_networkClient = net; // Non-owning; engine owns the connection.
    m_serverDisplayName = RANMobile::AuthenticationManager::GetServerDisplayName();
}

// TODO(user): input, focus, keyboard editing, layout and interaction behavior.
void LoginScreen::HandleTouchDown(int, float, float) {}
void LoginScreen::HandleTouchMove(int, float, float) {}
void LoginScreen::HandleTouchUp(int, float, float) {}
void LoginScreen::HandleTextInput(const std::string&) {}
void LoginScreen::HandleBackspace() {}
void LoginScreen::Update(float) {}

// Only authentication seam: do not reimplement packets/TEA in the UI.
// Existing Phase 3.4 manager owns opcode 2079, TEA and 56-byte login framing.
// true means sent/queued, NOT that server authentication succeeded.
bool LoginScreen::OnLoginPressed() {
    return RANMobile::AuthenticationManager::Login(m_networkClient, m_username, m_password);
}

// TODO(user): drawing. Intentionally no renderer calls, theme values or effects.
void LoginScreen::Draw() {}
void LoginScreen::Draw(RANMobile::MobileRenderer*) {}
