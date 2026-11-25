#include "AuthenticationManager.h"
#include "../../Engine/Network/NetworkClient.h"
#include "../../Engine/Network/EP7Protocol.h"
#include "../../Engine/Network/EP7Tea.h"
#include "../../Engine/Core/Logger.h"
#include <cstring>
#include <algorithm>

namespace RANMobile {

// Mirrors CNetClient::ThaiSndLogin (netclientLib/s_NetClientMsgLogin.cpp:282):
//   THAI_NET_LOGIN_DATA tnld; tnld.nChannel = nChannel;
//   StringCchCopy(tnld.szUserid, 21, userid);
//   StringCchCopy(tnld.szPassword, 21, userpass);
//   m_Tea.encrypt(tnld.szUserid, 21);
//   m_Tea.encrypt(tnld.szPassword, 21);
//   return Send((char*)&tnld);
// PC Send() transmits nmg.dwSize (56) raw struct bytes
// (netclientLib/s_NetClient.cpp:827-830). NetworkClient::SendPacket builds
// the same wire bytes: header {dwSize=56, nType=2079} + 48-byte payload.
bool AuthenticationManager::Login(NetworkClient* net, const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        Logger::Warn("AuthenticationManager", "Username or password is empty");
        return false;
    }
    if (!net || !net->IsConnected()) {
        Logger::Warn("AuthenticationManager", "Not connected to EP7 Login Server (" + s_serverDisplayName + ")");
        return false;
    }

    EP7ThaiNetLoginData loginPkt;
    loginPkt.nChannel = 0; // default channel (PC LoginPage passes server channel)

    // StringCchCopy equivalent: truncate to 20 chars, always NUL-terminated.
    std::strncpy(loginPkt.szUserid, username.c_str(), EP7_USR_ID_LENGTH);
    loginPkt.szUserid[EP7_USR_ID_LENGTH] = '\0';
    std::strncpy(loginPkt.szPassword, password.c_str(), EP7_USR_PASS_LENGTH);
    loginPkt.szPassword[EP7_USR_PASS_LENGTH] = '\0';

    // TEA-encrypt the full 21-byte fields (PC: m_Tea.encrypt(field, 21)).
    EP7Tea tea; // default key "Steven Seagal Neck Break" (minTea.cpp:20)
    if (!tea.encrypt(loginPkt.szUserid, static_cast<int>(EP7_USR_ID_LENGTH + 1)) ||
        !tea.encrypt(loginPkt.szPassword, static_cast<int>(EP7_USR_PASS_LENGTH + 1))) {
        Logger::Error("AuthenticationManager", "TEA encrypt failed for login packet");
        return false;
    }

    // Send exact EP7 THAI_NET_MSG_LOGIN payload (wire size 56, dwSize=56).
    const uint8_t* payload = reinterpret_cast<const uint8_t*>(&loginPkt) + sizeof(EP7NetMsgGeneric);
    uint32_t payloadSize = static_cast<uint32_t>(sizeof(EP7ThaiNetLoginData) - sizeof(EP7NetMsgGeneric));

    Logger::Info("AuthenticationManager", "Sending EP7 THAI_NET_MSG_LOGIN for user: " + username + " -> " + s_serverDisplayName);
    return net->SendPacket(EP7_THAI_NET_MSG_LOGIN, payload, payloadSize);
}

} // namespace RANMobile
