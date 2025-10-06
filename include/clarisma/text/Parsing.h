// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>

namespace clarisma {
namespace Parsing {

template <typename Char>
requires std::is_integral_v<Char> && (sizeof(Char) <= 4)
uint64_t parseUnsignedLong(const Char* s)
{
    assert(s != nullptr);
    uint64_t v = 0;
    while (*s)
    {
        unsigned digit = static_cast<unsigned>(*s - '0');
        if (digit > 9) break;
        v = v * 10 + digit;
        ++s;
    }
    return v;
}

template <typename Char>
requires std::is_integral_v<Char> && (sizeof(Char) <= 4)
uint64_t parseUnsignedLong(const Char* s, const Char* end)
{
    assert(s != nullptr && end != nullptr);
    uint64_t v = 0;
    while (s < end)
    {
        unsigned digit = static_cast<unsigned>(*s - '0');
        if (digit > 9) break;
        v = v * 10 + digit;
        ++s;
    }
    return v;
}

inline uint64_t parseUnsignedLong(std::string_view sv)
{
    return parseUnsignedLong(sv.data(), sv.data() + sv.length());
}

} // namespace Parsing
} // namespace clarisma
