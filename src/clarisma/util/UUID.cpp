// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/util/UUID.h>
#include <random>
#include <clarisma/text/Format.h>


namespace clarisma {

UUID UUID::create()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);
    uint8_t guid[16];

    // Generate two 64-bit random numbers
    uint64_t r1 = dis(gen);
    uint64_t r2 = dis(gen);

    // Fill the first 8 bytes with r1
    for (int i = 0; i < 8; ++i)
    {
        guid[i] = static_cast<uint8_t>(r1 >> (i * 8));
    }

    // Fill the last 8 bytes with r2
    for (int i = 0; i < 8; ++i)
    {
        guid[i + 8] = static_cast<uint8_t>(r2 >> (i * 8));
    }

    // Set the version (4) and variant (RFC 4122)
    guid[6] = (guid[6] & 0x0F) | 0x40;  // version 4
    guid[8] = (guid[8] & 0x3F) | 0x80;  // variant 1
    return UUID(guid);
}


char* UUID::format(char* buf) const
{
    std::snprintf(
        buf, 37, // 36 characters + null terminator
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        guid_[3], guid_[2], guid_[1], guid_[0],   // Data1 (little-endian)
        guid_[5], guid_[4],                       // Data2 (little-endian)
        guid_[7], guid_[6],                       // Data3 (little-endian)
        guid_[8], guid_[9],                       // Data4 (big-endian)
        guid_[10], guid_[11], guid_[12], guid_[13], guid_[14], guid_[15] // Data4 (big-endian)
    );
    return buf+36;
}

} // namespace clarisma
