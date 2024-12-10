// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>

namespace clarisma {

namespace Hash
{
    inline size_t combine(size_t seed, size_t value)
    {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }

    inline size_t hash(const uint64_t* data, size_t size)
    {
        size_t seed = 0;
        for (size_t i = 0; i < size; ++i)
        {
            seed = combine(seed, data[i]);
        }
        return seed;
    }
};

} // namespace clarisma
