// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>

namespace clarisma {

struct BTreeData
{
    using PageNum = uint32_t;

    PageNum root = 0;
    uint32_t height = 0;
};

} // namespace clarisma
