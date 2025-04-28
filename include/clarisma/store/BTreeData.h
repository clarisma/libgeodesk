// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>

namespace clarisma {

struct BTreeData
{
    using PageNum = uint32_t;

    struct Entry
    {
        union
        {
            uint32_t key;
            uint32_t count;
        };
        union
        {
            uint32_t value;
            uint32_t child;
        };

        bool operator==(const Entry& other) const noexcept
        {
            return key == other.key && value == other.value;
        }

        bool operator<(const Entry& other) const noexcept
        {
            return (key < other.key) || (key == other.key && value < other.value);
        }
    };

    PageNum root = 0;
    uint32_t height = 0;
};

} // namespace clarisma
