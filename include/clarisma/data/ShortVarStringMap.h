// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/data/HashMap.h>
#include <clarisma/util/ShortVarString.h>
#include <functional>

namespace clarisma {

// Hash for ShortVarString*, with heterogeneous lookup support
struct ShortVarStringHash
{
    using is_transparent = void;

    size_t operator()(const ShortVarString* s) const noexcept
    {
        return std::hash<std::string_view>{}(s->toStringView());
    }

    size_t operator()(std::string_view sv) const noexcept
    {
        return std::hash<std::string_view>{}(sv);
    }

    size_t operator()(const char* s) const noexcept
    {
        return std::hash<std::string_view>{}(
            std::string_view{s, std::strlen(s)});
    }
};

// Equality for ShortVarString*, with heterogeneous lookup support
struct ShortVarStringEq
{
    using is_transparent = void;

    bool operator()(const ShortVarString* a,
                    const ShortVarString* b) const noexcept
    {
        return *a == *b;
    }

    bool operator()(const ShortVarString* a,
                    std::string_view b) const noexcept
    {
        return *a == b;
    }

    bool operator()(std::string_view a,
                    const ShortVarString* b) const noexcept
    {
        return *b == a;
    }

    bool operator()(const ShortVarString* a,
                    const char* b) const noexcept
    {
        return a->equals(b, std::strlen(b));
    }

    bool operator()(const char* a,
                    const ShortVarString* b) const noexcept
    {
        return b->equals(a, std::strlen(a));
    }
};

// Convenience alias: HashMap with ShortVarString* as key
template<class V>
using ShortVarStringMap =
    HashMap<const ShortVarString*, V, ShortVarStringHash, ShortVarStringEq>;

} // namespace clarisma
