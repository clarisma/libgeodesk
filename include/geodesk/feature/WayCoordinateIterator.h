// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/WayPtr.h>

namespace geodesk {

///
/// \cond lowlevel
///
// TODO: could store the flags in this struct, there is room anyway due to padding
class WayCoordinateIterator
{
public:
    WayCoordinateIterator() {};
    explicit WayCoordinateIterator(WayPtr way);

    // TODO: Fix the flags vs. bool issue, because it could lead to unintended behavior:
    // flags can be silently passed as duplicateFirst, so even if a feature is not an
    // area, it will be treated as such since area-flag is not isolated
    // Better to make FeatureFlags a type-safe enum class!
    void start(const uint8_t* p, int32_t prevX, int32_t prevY, bool duplicateFirst);
    void start(FeaturePtr way, int flags);
    Coordinate next();
    Coordinate current() const { return Coordinate(x_, y_); }

    // does not include any duplicated last coordinate
    int storedCoordinatesRemaining() const { return remaining_; }
    // This one includes any duplicate last coordinate for areas, based on flags:
    int coordinatesRemaining() const
    {
        return remaining_ + (duplicateFirst_ ? 1 : 0);
    }

    const uint8_t* wayNodeIDs() const
    {
        const uint8_t* pIDs = p_;
        clarisma::skipVarints(pIDs, (remaining_-1) * 2);
        return pIDs;
    }

private:
    const uint8_t* p_;
    int remaining_;
    bool duplicateFirst_;
    int32_t x_;				// TODO: use Coordinate
    int32_t y_;
    int32_t firstX_;
    int32_t firstY_;
};

// \endcond
} // namespace geodesk
