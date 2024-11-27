// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>
#include <string_view>

namespace clarisma {

class TextMetrics 
{
public:
    // Counts the number of UTF-8 characters in the given string.
    static size_t countCharsUtf8(std::string_view s) noexcept
    {
        size_t count = 0;
        for (uint8_t byte : s)
        {
            if (isUtf8StartByte(byte)) ++count;
        }
        return count;
    }

    static size_t countCharsUtf8(const char* s) noexcept
    {
        size_t count = 0;
        char ch;
        while((ch = *s++) != 0)
        {
            if (isUtf8StartByte(ch)) ++count;
        }
        return count;
    }

private:
    // Helper to determine if a byte is the start of a UTF-8 character.
    static bool isUtf8StartByte(uint8_t byte) noexcept
    {
        return (byte & 0xC0) != 0x80;
    }
};

} // namespace clarisma
