#pragma once

#include <cstdint>

namespace RANMobile {

// Port of netserverLib/minTea.h + minTea.cpp (PC source, ground truth).
// TEA (Tiny Encryption Algorithm) by David Wheeler & Roger Needham,
// as adapted by Andreas Jonsson and modified by Jgkim (2005-11-03).
//
// Default key from PC source (minTea.cpp:20):
//   char* szKey = "Steven Seagal Neck Break";
// setKey() repeats the key string to fill TEA_KEY_LENGTH (16) bytes:
//   realKey[n] = szKey[n % keyLen]
// so the effective 16-byte key is "Steven Seagal Ne" (first 16 chars).
//
// Wire behavior (must match PC byte-for-byte):
//   encrypt(szData, nMaxLength): trim trailing NULs -> pad to >4 bytes and
//     multiple of 4 -> TEA-encrypt UINT words -> copy back (length + NUL).
//   decrypt(szData, nMaxLength): trim trailing NULs -> pad UP to multiple
//     of 4 -> TEA-decrypt UINT words -> copy back.
// Words are native uint32_t (little-endian on x86/x64/ARM64 — same as PC).
class EP7Tea {
public:
    static constexpr int kKeyLength = 16; // TEA_KEY_LENGTH (minTea.h)
    static constexpr const char* kDefaultKey = "Steven Seagal Neck Break";

    EP7Tea();
    explicit EP7Tea(const char* szKey);

    bool setKey(const char* szKey);
    bool encrypt(char* szData, int nMaxLength);
    bool decrypt(char* szData, int nMaxLength);

    // Exposed for unit-test verification of key derivation.
    const uint8_t* keyBytes() const { return m_realKey; }

private:
    int encryptWords(uint32_t* v, uint32_t n, uint32_t* k);
    int decryptWords(uint32_t* v, uint32_t n, uint32_t* k);

    uint8_t m_realKey[kKeyLength]{};
};

} // namespace RANMobile
