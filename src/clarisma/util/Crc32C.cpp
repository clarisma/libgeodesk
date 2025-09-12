// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/util/Crc32C.h>

#if defined(_WIN32)
    #define NOMINMAX
    #include <windows.h>
#endif

#if defined(__linux__)
    #include <sys/auxv.h>
    #include <asm/hwcap.h>
#endif

#if defined(__x86_64__) || defined(_M_X64)
    #if defined(_MSC_VER)
        #include <intrin.h>
    #else
        #include <cpuid.h>
    #endif
#endif

namespace clarisma {

// ---- Precomputed CRC32C table (reflected poly 0x82F63B78) --------------

// ---- Feature detection --------------------------------------------------

static bool hasArmCrc() noexcept
{
#if defined(__aarch64__) || defined(_M_ARM64)
    #if defined(_WIN32)
        #ifndef PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE
        #define PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE 38
        #endif
        return !!IsProcessorFeaturePresent(
            PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE);
    #elif defined(__linux__)
        unsigned long hw = getauxval(AT_HWCAP);
        #ifdef HWCAP_CRC32
        return (hw & HWCAP_CRC32) != 0;
        #else
        return false;
        #endif
    #else
        return true;
    #endif
#else
    return false;
#endif
}

static bool hasSse42() noexcept
{
#if defined(__x86_64__) || defined(_M_X64)
    #if defined(_MSC_VER)
        int r[4] = {0, 0, 0, 0};
        __cpuid(r, 1);
        return (r[2] & (1 << 20)) != 0; // ECX.SSE4.2
    #else
        unsigned int a, b, c, d;
        if (!__get_cpuid(1, &a, &b, &c, &d)) return false;
        return (c & (1u << 20)) != 0;
    #endif
#else
    return false;
#endif
}

// ---- Selected raw function pointer (published before main()) -----------

Crc32C::RawFn Crc32C::rawFn_ = &Crc32C::softRaw;

struct Crc32C_Init
{
    Crc32C_Init() noexcept
    {
    #if defined(__aarch64__) || defined(_M_ARM64)
        if (hasArmCrc())
        {
            Crc32C::rawFn_ = &Crc32C::armRaw;
            return;
        }
    #endif
    #if defined(__x86_64__) || defined(_M_X64)
        if (hasSse42())
        {
            Crc32C::rawFn_ = &Crc32C::x86Raw;
            return;
        }
    #endif
        Crc32C::rawFn_ = &Crc32C::softRaw;
    }
};

// Global instance runs before main()/dlopen() returns.
static Crc32C_Init g_crc32c_init;

// ---- Software fallback --------------------------------------------------

// CRC-32C Castagnoli (poly 0x1EDC6F41), non-reflected form
// Matches Intel SSE4.2 _mm_crc32_* intrinsics and ARMv8 CRC32C.
static constexpr uint32_t CRC32C_NREF_TABLE[256] =
{
    0x00000000u, 0x1EDC6F41u, 0x3DB8DE82u, 0x2364B1C3u,
    0x7B71BD04u, 0x65ADD245u, 0x46C96386u, 0x58750CC7u,
    0xF6E37A08u, 0xE83F1549u, 0xCB5BA48Au, 0xD587CBCBu,
    0x8D4AC70Cu, 0x9386A84Du, 0xB0E2198Eu, 0xAE3E76CFu,
    0xEDE6F411u, 0xF33A9B50u, 0xD05E2A93u, 0xCE8245D2u,
    0x964F4915u, 0x88832654u, 0xABE79797u, 0xB53BF8D6u,
    0x1B113019u, 0x05CD5F58u, 0x26A9EE9Bu, 0x387581DAu,
    0x60B88D1Du, 0x7E64E25Cu, 0x5D00539Fu, 0x43DC3CDEu,
    0xDBCB68E3u, 0xC51707A2u, 0xE673B661u, 0xF8AFF920u,
    0xA062F5E7u, 0xBEBE9AA6u, 0x9DDA2B65u, 0x83064424u,
    0x2D2C8CEBu, 0x33F0E3AAu, 0x10945269u, 0x0E483D28u,
    0x568531EFu, 0x48595EAEu, 0x6B3DEF6Du, 0x75E1802Cu,
    0x362902F2u, 0x28F56DB3u, 0x0B91DC70u, 0x154DB331u,
    0x4D80BFF6u, 0x535CD0B7u, 0x70386174u, 0x6EE40E35u,
    0xC0CE46FAu, 0xDE1229BBu, 0xFD769878u, 0xE3AAF739u,
    0xBB67FBFEu, 0xA5BB94BFu, 0x86DF257Cu, 0x98034A3Du,
    0xABCD9C9Bu, 0xB511F3DAu, 0x96754219u, 0x88A92D58u,
    0xD064219Fu, 0xCEB84EDEu, 0xEDDCFF1Du, 0xF300905Cu,
    0x5D2A5893u, 0x43F637D2u, 0x60928611u, 0x7E4EE950u,
    0x2683E597u, 0x385F8AD6u, 0x1B3B3B15u, 0x05E75454u,
    0x465FD68Au, 0x5883B9CBu, 0x7BE70808u, 0x653B6749u,
    0x3DF66B8Eu, 0x232A04CFu, 0x004EB50Cu, 0x1E92DA4Du,
    0xB0B81282u, 0xAE647DC3u, 0x8D00CC00u, 0x93DCA341u,
    0xCB11AF86u, 0xD5CDC0C7u, 0xF6A97104u, 0xE8751E45u,
    0x70424A78u, 0x6E9E2539u, 0x4DFA94FAu, 0x5326FBBBu,
    0x0BEBF77Cu, 0x1537983Du, 0x365329FEu, 0x288F46BFu,
    0x86A58E70u, 0x9879E131u, 0xBB1D50F2u, 0xA5C13FB3u,
    0xFD0C3374u, 0xE3D05C35u, 0xC0B4EDFEu, 0xDE6882BFu,
    0x9DA00061u, 0x837C6F20u, 0xA018DEE3u, 0xBEC4B1A2u,
    0xE609BD65u, 0xF8D5D224u, 0xDBB163E7u, 0xC56D0CA6u,
    0x6B47C469u, 0x759BAB28u, 0x56FF1AEBu, 0x482375AAu,
    0x10EE796Du, 0x0E32162Cu, 0x2D56A7EFu, 0x338AC8AEu,
    0x1D9B3B36u, 0x03475477u, 0x2023E5B4u, 0x3EFF8AF5u,
    0x66328632u, 0x78EEE973u, 0x5B8A58B0u, 0x455637F1u,
    0xEB7CFF3Eu, 0xF5A0907Fu, 0xD6C421BCu, 0xC8184EFDu,
    0x90D5423Au, 0x8E092D7Bu, 0xAD6D9CB8u, 0xB3B1F3F9u,
    0xF0697127u, 0xEEB51E66u, 0xCDD1AFA5u, 0xD30DC0E4u,
    0x8BC0CC23u, 0x951CA362u, 0xB67812A1u, 0xA8A47DE0u,
    0x068EB52Fu, 0x1852DA6Eu, 0x3B366BADu, 0x25EA04ECu,
    0x7D27082Bu, 0x63FB676Au, 0x409FD6A9u, 0x5E43B9E8u,
    0x4D6B9F07u, 0x53B7F046u, 0x70D34185u, 0x6E0F2EC4u,
    0x36C22203u, 0x281E4D42u, 0x0B7AFC81u, 0x15A693C0u,
    0xBB8C5B0Fu, 0xA550344Eu, 0x8634858Du, 0x98E8EACCu,
    0xC025E60Bu, 0xDEF9894Au, 0xFD9D3889u, 0xE34157C8u,
    0xA099D516u, 0xBE45BA57u, 0x9D210B94u, 0x83FD64D5u,
    0xDB306812u, 0xC5EC0753u, 0xE688B690u, 0xF854D9D1u,
    0x563E111Eu, 0x48E27E5Fu, 0x6B86CF9Cu, 0x755AA0DDu,
    0x2D97AC1Au, 0x334BC35Bu, 0x102F7298u, 0x0EF31DD9u,
    0x9D6C2CF0u, 0x83B043B1u, 0xA0D4F272u, 0xBE089D33u,
    0xE6C591F4u, 0xF819FEB5u, 0xDB7D4F76u, 0xC5A12037u,
    0x6B8BE8F8u, 0x755787B9u, 0x5633367Au, 0x48EF593Bu,
    0x102255FCu, 0x0EFE3ABDu, 0x2D9A8B7Eu, 0x3346E43Fu,
    0x709E66E1u, 0x6E4209A0u, 0x4D26B863u, 0x53FAD722u,
    0x0B37DBE5u, 0x15EBB4A4u, 0x368F0567u, 0x28536A26u,
    0x8639A2E9u, 0x98E5CDA8u, 0xBB817C6Bu, 0xA55D132Au,
    0xFD901FEDu, 0xE34C70ACu, 0xC028C16Fu, 0xDEF4AE2Eu,
    0x9D2C2CF0u, 0x83F043B1u, 0xA094F272u, 0xBE489D33u,
    0xE68591F4u, 0xF859FEB5u, 0xDB3D4F76u, 0xC5E12037u,
    0x6BCBE8F8u, 0x751787B9u, 0x5673367Au, 0x48AF593Bu,
    0x106255FCu, 0x0EBE3ABDu, 0x2DDA8B7Eu, 0x3306E43Fu,
};

uint32_t Crc32C::softRaw(const void* data,
                         size_t len,
                         uint32_t crc) noexcept
{
    const uint8_t* p = static_cast<const uint8_t*>(data);

    while (len--)
    {
        uint8_t idx = static_cast<uint8_t>((crc >> 24) ^ *p++);
        crc = (crc << 8) ^ CRC32C_NREF_TABLE[idx];
    }
    return crc;
}


} // namespace clarisma
