// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdint>
#include <geodesk/feature/FeatureType.h>

namespace geodesk {

/// \cond lowlevel
///
class FeatureHeader
{
public:
    explicit FeatureHeader(uint64_t bits) : bits_(bits) {}

    // TODO: This will change in 2.0
    static FeatureHeader forTypeAndId(FeatureType type, uint64_t id)
    {
        return FeatureHeader((id << 12) | (static_cast<int>(type) << 3));
    }

    uint64_t bits() const noexcept { return bits_; }

    uint64_t id() const noexcept
    {
        return bits_ >> 12;
    }

    int flags() const noexcept
    {
        return static_cast<int>(bits_);
    }

    int typeCode() const
    {
        return static_cast<int>(bits_ >> 3) & 3;
    }

private:
    uint64_t bits_;
};

// \endcond

} // namespace geodesk
