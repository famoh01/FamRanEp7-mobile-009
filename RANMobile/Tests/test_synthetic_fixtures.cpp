// SYNTHETIC_TEST_FIXTURE — structural layout only, no game content,
// generated to validate parser mechanics against documented spec
//
// Parser regression tests for .wld/.chf/.cps/.dds/.mtf using synthetic
// binary fixtures. No real game assets. No game-content strings.

#include "../Data/Importer/EP7WorldCharParser.h"
#include "../Data/Importer/EP7TextureDecoder.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),
                                std::istreambuf_iterator<char>());
}

std::vector<uint8_t> loadFixture(const std::string& name) {
    const std::string bases[] = {
        "Tests/Fixtures/",
        "RANMobile/Tests/Fixtures/",
        "../Tests/Fixtures/",
        "./Tests/Fixtures/"
    };
    for (const auto& b : bases) {
        auto v = readFile(b + name);
        if (!v.empty()) return v;
    }
    std::cerr << "MISSING FIXTURE: " << name << "\n";
    return {};
}

void testWld() {
    using RANMobile::EP7WorldCharParser;
    // Valid: version 0x0108 (min supported)
    {
        auto data = loadFixture("synthetic_wld_valid.bin");
        assert(data.size() == 132);
        auto hdr = EP7WorldCharParser::ParseWldHeader(data);
        assert(hdr.valid);
        assert(hdr.version == 0x0108);
        assert(!hdr.fileType.empty());
        // fileType is a synthetic placeholder, not game content
        assert(hdr.fileType.find("TEST_FIXTURE") != std::string::npos);
    }
    // Invalid: version below min
    {
        auto data = loadFixture("synthetic_wld_invalid_version.bin");
        auto hdr = EP7WorldCharParser::ParseWldHeader(data);
        assert(!hdr.valid);
        assert(!hdr.error.empty());
    }
    // Invalid: truncated
    {
        auto data = loadFixture("synthetic_wld_truncated.bin");
        auto hdr = EP7WorldCharParser::ParseWldHeader(data);
        assert(!hdr.valid);
        assert(!hdr.error.empty());
    }
    std::cout << "WLD: PASS\n";
}

void testChf() {
    using RANMobile::EP7WorldCharParser;
    // Valid versions 0x0100-0x0103
    const uint32_t validVersions[] = {0x0100, 0x0101, 0x0102, 0x0103};
    for (uint32_t v : validVersions) {
        char name[64];
        std::snprintf(name, sizeof(name), "synthetic_chf_valid_%#06x.bin", v);
        auto data = loadFixture(name);
        assert(data.size() == 4);
        auto hdr = EP7WorldCharParser::ParseChfHeader(data);
        assert(hdr.valid);
        assert(hdr.version == v);
    }
    // Invalid version
    {
        auto data = loadFixture("synthetic_chf_invalid_version.bin");
        auto hdr = EP7WorldCharParser::ParseChfHeader(data);
        assert(!hdr.valid);
    }
    // Truncated
    {
        auto data = loadFixture("synthetic_chf_truncated.bin");
        auto hdr = EP7WorldCharParser::ParseChfHeader(data);
        assert(!hdr.valid);
    }
    std::cout << "CHF: PASS\n";
}

void testCps() {
    using RANMobile::EP7WorldCharParser;
    // Valid min/max
    {
        auto data = loadFixture("synthetic_cps_valid_min.bin");
        auto hdr = EP7WorldCharParser::ParseCpsHeader(data);
        assert(hdr.valid);
        assert(hdr.version == 1);
    }
    {
        auto data = loadFixture("synthetic_cps_valid_max.bin");
        auto hdr = EP7WorldCharParser::ParseCpsHeader(data);
        assert(hdr.valid);
        assert(hdr.version == 0x00000107);
    }
    // Invalid zero
    {
        auto data = loadFixture("synthetic_cps_invalid_zero.bin");
        auto hdr = EP7WorldCharParser::ParseCpsHeader(data);
        assert(!hdr.valid);
    }
    // Invalid over max
    {
        auto data = loadFixture("synthetic_cps_invalid_over.bin");
        auto hdr = EP7WorldCharParser::ParseCpsHeader(data);
        assert(!hdr.valid);
    }
    // Truncated
    {
        auto data = loadFixture("synthetic_cps_truncated.bin");
        auto hdr = EP7WorldCharParser::ParseCpsHeader(data);
        assert(!hdr.valid);
    }
    std::cout << "CPS: PASS\n";
}

void testDds() {
    using RANMobile::EP7TextureDecoder;
    // Valid minimal DXT1 4x4
    {
        auto data = loadFixture("synthetic_dds_valid_dxt1_4x4.bin");
        assert(data.size() == 136);
        auto tex = EP7TextureDecoder::DecodeDDS(data);
        assert(tex.valid);
        assert(tex.width == 4);
        assert(tex.height == 4);
        assert(tex.rgba8.size() == 4u * 4u * 4u);
        // DXT1 block: color0=white, color1=black, all indices 0 -> all white
        for (size_t i = 0; i < tex.rgba8.size(); i += 4) {
            assert(tex.rgba8[i + 0] == 255);
            assert(tex.rgba8[i + 1] == 255);
            assert(tex.rgba8[i + 2] == 255);
            assert(tex.rgba8[i + 3] == 255);
        }
    }
    // Invalid magic
    {
        auto data = loadFixture("synthetic_dds_invalid_magic.bin");
        auto tex = EP7TextureDecoder::DecodeDDS(data);
        assert(!tex.valid);
    }
    // Truncated
    {
        auto data = loadFixture("synthetic_dds_truncated.bin");
        auto tex = EP7TextureDecoder::DecodeDDS(data);
        assert(!tex.valid);
    }
    // Invalid zero dimension
    {
        auto data = loadFixture("synthetic_dds_invalid_zero_dim.bin");
        auto tex = EP7TextureDecoder::DecodeDDS(data);
        assert(!tex.valid);
    }
    std::cout << "DDS: PASS\n";
}

void testMtf() {
    using RANMobile::EP7TextureDecoder;
    // Known plaintext/ciphertext pair (synthetic, sequential bytes - no game content)
    const uint8_t knownPlain[] = {0x00,0x01,0x02,0x03,0x10,0x20,0x30,0x40};
    const size_t plainLen = sizeof(knownPlain);

    // Valid decrypt
    {
        auto data = loadFixture("synthetic_mtf_valid.bin");
        assert(data.size() == 12 + plainLen);
        std::vector<uint8_t> outRaw;
        int outFileType = -1;
        bool ok = EP7TextureDecoder::DecryptMTF(data, outRaw, outFileType);
        assert(ok);
        assert(outFileType == 7);
        assert(outRaw.size() == plainLen);
        assert(std::memcmp(outRaw.data(), knownPlain, plainLen) == 0);
    }

    // Round-trip proof: encrypt known plain with inverse formula, compare to fixture cipher
    {
        auto data = loadFixture("synthetic_mtf_valid.bin");
        // extract cipher bytes from fixture
        std::vector<uint8_t> cipher(data.begin() + 12, data.end());
        assert(cipher.size() == plainLen);
        // inverse of decrypt (b = b + DIFF; b = b ^ XOR) is (enc = (plain ^ XOR) - DIFF)
        std::vector<uint8_t> reEnc(plainLen);
        for (size_t i = 0; i < plainLen; ++i) {
            uint8_t b = static_cast<uint8_t>(knownPlain[i] ^ EP7TextureDecoder::TEX_XOR_DATA);
            b = static_cast<uint8_t>(b - EP7TextureDecoder::TEX_DIFF_DATA);
            reEnc[i] = b;
        }
        assert(reEnc == cipher);
        // and decrypt of reEnc (via full MTF) yields plain again
        std::vector<uint8_t> rebuiltMtf(12 + plainLen);
        int32_t ver = 1; int32_t sz = static_cast<int32_t>(plainLen); int32_t ft = 7;
        std::memcpy(rebuiltMtf.data(), &ver, 4);
        std::memcpy(rebuiltMtf.data() + 4, &sz, 4);
        std::memcpy(rebuiltMtf.data() + 8, &ft, 4);
        std::memcpy(rebuiltMtf.data() + 12, reEnc.data(), plainLen);
        std::vector<uint8_t> outRaw2;
        int ft2 = -1;
        assert(EP7TextureDecoder::DecryptMTF(rebuiltMtf, outRaw2, ft2));
        assert(outRaw2.size() == plainLen);
        assert(std::memcmp(outRaw2.data(), knownPlain, plainLen) == 0);
    }

    // Invalid: nSize too large
    {
        auto data = loadFixture("synthetic_mtf_invalid_size.bin");
        std::vector<uint8_t> outRaw;
        int outFileType = -1;
        bool ok = EP7TextureDecoder::DecryptMTF(data, outRaw, outFileType);
        assert(!ok);
    }
    // Truncated (<= header size)
    {
        auto data = loadFixture("synthetic_mtf_truncated.bin");
        std::vector<uint8_t> outRaw;
        int outFileType = -1;
        bool ok = EP7TextureDecoder::DecryptMTF(data, outRaw, outFileType);
        assert(!ok);
    }
    std::cout << "MTF: PASS\n";
}

} // namespace

int main() {
    std::cout << "=== Synthetic Fixture Parser Regression Tests ===\n";
    std::cout << "SYNTHETIC_TEST_FIXTURE: structural layout only, no game content\n";
    testWld();
    testChf();
    testCps();
    testDds();
    testMtf();
    std::cout << "ALL SYNTHETIC FIXTURE TESTS: PASS\n";
    return 0;
}
