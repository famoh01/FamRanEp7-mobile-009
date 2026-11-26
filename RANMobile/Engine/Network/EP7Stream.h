#pragma once
#include "EP7Protocol.h"
#include "EP7Tea.h"
#include <algorithm>
#include <array>
#include <vector>

namespace RANMobile {
// Raw EP7 framing: dwSize includes the eight-byte header. PC NET_DATA_BUFSIZE=1024.
// One incomplete frame at most; caller drains completed frames before feeding more.
class EP7Stream {
public:
    bool Push(uint8_t byte, std::vector<uint8_t>& frame) {
        frame.clear();
        m_bytes.push_back(byte);
        if (m_bytes.size() < sizeof(EP7NetMsgGeneric)) return true;
        EP7NetMsgGeneric header{};
        std::memcpy(&header, m_bytes.data(), sizeof(header));
        if (header.dwSize < sizeof(header) || header.dwSize > 1024) {
            Reset();
            return false;
        }
        if (m_bytes.size() == header.dwSize) frame.swap(m_bytes);
        return true;
    }
    void Reset() { m_bytes.clear(); }
private:
    std::vector<uint8_t> m_bytes;
};

// PC SndHeartbeat: Login/Field return a zero key; Agent encrypts mutable key
// on EVERY request. Receiving NET_MSG_SND_ENCRYPT_KEY selects the Agent path.
class EP7Heartbeat {
public:
    void Reset() { m_agent = false; m_key.fill(0); }
    bool Handle(int32_t type, const uint8_t* frame, size_t size,
                std::vector<uint8_t>& answer) {
        answer.clear();
        if (type == EP7_NET_MSG_SND_ENCRYPT_KEY) {
            if (size != 24) return false;
            m_key.fill(0);
            for (size_t i = 0; i < 12 && frame[8 + i] != 0; ++i)
                m_key[i] = static_cast<char>(frame[8 + i]);
            m_agent = true;
        } else if (type == EP7_NET_MSG_HEARTBEAT_CLIENT_REQ) {
            if (size != 8) return false;
            // Native PC struct: header(8)+CHAR[13]+tail padding(3)=24.
            answer.assign(24, 0);
            const EP7NetMsgGeneric header{24, EP7_NET_MSG_HEARTBEAT_CLIENT_ANS};
            std::memcpy(answer.data(), &header, sizeof(header));
            if (m_agent) {
                EP7Tea tea;
                tea.encrypt(m_key.data(), static_cast<int>(m_key.size()));
                std::memcpy(answer.data() + 8, m_key.data(), m_key.size());
            }
        }
        return true;
    }
private:
    bool m_agent = false;
    std::array<char, 13> m_key{};
};
} // namespace RANMobile
