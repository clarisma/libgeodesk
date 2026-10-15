// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/geom/index/hilbert.h>
#include <geodesk/geom/Tile.h>

namespace geodesk {

class HilbertDistanceInTile
{
public:
    HilbertDistanceInTile(Tile tile) :
        tileLeft_(tile.leftX()),
        tileBottom_(tile.bottomY()),
        zoomDelta_(16 - tile.zoom())
    {
    }

    uint32_t compute(Coordinate xy) const noexcept
    {
        int32_t x = std::clamp((xy.x - tileLeft_) >> zoomDelta_, 0, hilbert::MAX_COORDINATE);
        int32_t y = std::clamp((xy.y - tileBottom_) >> zoomDelta_, 0, hilbert::MAX_COORDINATE);
        return hilbert::calculateHilbertDistance(
            static_cast<uint32_t>(x), static_cast<uint32_t>(y));
    }

private:
    int32_t tileLeft_;
    int32_t tileBottom_;
    int zoomDelta_;
};

} // namespace geodesk
