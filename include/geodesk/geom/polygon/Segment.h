// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/geom/polygon/Polygonizer.h>
#include <clarisma/alloc/Arena.h>

namespace geodesk {

class Polygonizer::Segment
{
public:
    // The order matters for these constants
    enum
    {
        SEGMENT_UNASSIGNED = 0,
        SEGMENT_TENTATIVE = 1,
        SEGMENT_ASSIGNED = 2,
        SEGMENT_DANGLING = 3,
    };

    Segment* next;
    WayPtr way;
    bool backward;
    uint8_t status;
    uint16_t vertexCount;      // TODO: 16 bits enough? No more than 64K per segment 
    Coordinate coords[1];      // variable length

    /*
    Coordinate start() const { return coords[0]; }
    Coordinate end()   const { return coords[vertexCount-1]; }
    */

    static size_t sizeWithVertexCount(int vertexCount)
    {
        return sizeof(Segment) + sizeof(Coordinate) * (vertexCount - 1);
        // -1 because the placeholder is already part of Segment's basic size
    }

    bool isClosed() const
    {
        return coords[0] == coords[vertexCount - 1];
    }

    Box bounds() const
    {
        return way.bounds();
    }

    #ifdef GEODESK_WITH_GEOS
    void copyTo(GEOSContextHandle_t context, GEOSCoordSequence* seq, int destPos) const;
    #endif
    #ifdef GEODESK_WITH_OGR
    void copyTo(OGRSimpleCurve* curve, int destPos) const;
    #endif
    Segment* createFragment(int start, int end, clarisma::Arena& arena) const;
};



} // namespace geodesk
