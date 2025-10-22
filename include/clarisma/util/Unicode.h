// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <string>

namespace clarisma {

class Unicode
{
public:
    static void encode(char*& p, int code)
    {
        if (code <= 0x7F)
        {
            // 1-byte sequence: 0xxxxxxx
            *p++ = static_cast<char>(code);
        }
        else if (code <= 0x7FF)
        {
            // 2-byte sequence: 110xxxxx 10xxxxxx
            *p++ = static_cast<char>(0xC0 | (code >> 6));
            *p++ = static_cast<char>(0x80 | (code & 0x3F));
        }
        else if (code <= 0xFFFF)
        {
            // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
            *p++ = static_cast<char>(0xE0 | (code >> 12));
            *p++ = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            *p++ = static_cast<char>(0x80 | (code & 0x3F));
        }
        else if (code <= 0x10FFFF)
        {
            // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            *p++ = static_cast<char>(0xF0 | (code >> 18));
            *p++ = static_cast<char>(0x80 | ((code >> 12) & 0x3F));
            *p++ = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            *p++ = static_cast<char>(0x80 | (code & 0x3F));
        }
        else
        {
            // Invalid code point, you may handle this as needed
            // e.g., skip, insert replacement character, throw an exception, etc.
        }
    }

    static std::wstring toWideString(std::string_view s);
};

} // namespace clarisma
