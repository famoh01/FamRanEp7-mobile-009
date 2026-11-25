#include "EP7Tea.h"

#include <cmath>
#include <cstring>
#include <vector>

namespace RANMobile {

EP7Tea::EP7Tea() {
    setKey(kDefaultKey);
}

EP7Tea::EP7Tea(const char* szKey) {
    setKey(szKey);
}

bool EP7Tea::setKey(const char* szKey) {
    if (!szKey) return false;
    const size_t keyLen = std::strlen(szKey);
    if (keyLen == 0) return false;
    // PC: assert(strlen(szKey) > TEA_KEY_LENGTH); then repeat to 16 bytes.
    // Mobile: same repeat logic, but fail gracefully on empty key instead
    // of asserting (default key is 24 chars, always valid).
    std::memset(m_realKey, 0, kKeyLength);
    for (int n = 0; n < kKeyLength; ++n) {
        m_realKey[n] = static_cast<uint8_t>(szKey[n % keyLen]);
    }
    return true;
}

bool EP7Tea::encrypt(char* szData, int nMaxLength) {
    if (!szData || nMaxLength <= 0) return false;

    // Trim trailing NULs (minTea.cpp encrypt(char*, acCArray&, int)).
    int keyLen = nMaxLength;
    while (keyLen > 0) {
        if (szData[keyLen - 1] != 0) break;
        --keyLen;
    }

    std::vector<char> text(static_cast<size_t>(keyLen) + 1, 0);
    std::memcpy(text.data(), szData, static_cast<size_t>(keyLen));
    int len = keyLen;

    // The text must be larger than 4 bytes.
    while (len <= 4) {
        text[static_cast<size_t>(len++)] = 0;
    }
    // Pad with nulls until evenly divisible by 4.
    while (len & 3) {
        text[static_cast<size_t>(len++)] = 0;
    }

    encryptWords(reinterpret_cast<uint32_t*>(text.data()),
                 static_cast<uint32_t>(len / 4),
                 reinterpret_cast<uint32_t*>(m_realKey));

    // Encrypted length check (minTea.cpp encrypt(char*, int)).
    if (len > nMaxLength) return false;
    std::memcpy(szData, text.data(), static_cast<size_t>(len));
    szData[len] = 0;
    return true;
}

bool EP7Tea::decrypt(char* szData, int nMaxLength) {
    if (!szData || nMaxLength <= 0) return false;

    // Trim trailing NULs (minTea.cpp decrypt(char*, acCArray&, int)).
    int keyLen = nMaxLength;
    while (keyLen > 0) {
        if (szData[keyLen - 1] != 0) break;
        --keyLen;
    }
    // Pad UP to multiple of 4.
    while (keyLen & 3) ++keyLen;

    std::vector<char> text(static_cast<size_t>(keyLen) + 1, 0);
    std::memcpy(text.data(), szData, static_cast<size_t>(keyLen));

    decryptWords(reinterpret_cast<uint32_t*>(text.data()),
                 static_cast<uint32_t>(keyLen / 4),
                 reinterpret_cast<uint32_t*>(m_realKey));

    if (keyLen > nMaxLength) return false;
    std::memcpy(szData, text.data(), static_cast<size_t>(keyLen));
    szData[keyLen] = 0;
    return true;
}

int EP7Tea::encryptWords(uint32_t* v, uint32_t n, uint32_t* k) {
    if (n < 2) return -1;

    // TEA routine as per Wheeler & Needham, Oct 1998 (minTea.cpp).
    uint32_t z = v[n - 1], y = v[0], delta = 0x9E3779B9;
    uint32_t mx, e, q = static_cast<uint32_t>(std::floor(6 + 52.0f / n)), sum = 0, p;

    while (q-- > 0) { // 6 + 52/n operations gives between 6 & 32 mixes on each word
        sum += delta;
        e = sum >> 2 & 3;
        for (p = 0; p < n - 1; p++) {
            y = v[p + 1];
            mx = ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z));
            z = v[p] += mx;
        }
        y = v[0];
        mx = ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z));
        z = v[n - 1] += mx;
    }
    return 0;
}

int EP7Tea::decryptWords(uint32_t* v, uint32_t n, uint32_t* k) {
    if (n < 2) return -1;

    // TEA routine as per Wheeler & Needham, Oct 1998 (minTea.cpp).
    uint32_t z = v[n - 1], y = v[0], delta = 0x9E3779B9;
    uint32_t mx, e, q = static_cast<uint32_t>(std::floor(6 + 52.0f / n)), sum = q * delta, p;

    while (sum != 0) {
        e = sum >> 2 & 3;
        for (p = n - 1; p > 0; p--) {
            z = v[p - 1];
            mx = ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z));
            y = v[p] -= mx;
        }
        z = v[n - 1];
        mx = ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z));
        y = v[0] -= mx;
        sum -= delta;
    }
    return 0;
}

} // namespace RANMobile
