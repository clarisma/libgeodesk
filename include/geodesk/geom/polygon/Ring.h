// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/geom/polygon/Polygonizer.h>
#include <geodesk/geom/polygon/PointInPolygon.h>

namespace geodesk {

// TODO: Why won't Doxygen include this class?!

/// @brief An inner or outer ring produced by the Polygonizer.
/// Rings form a linked list; use `next()` to obtain the next ring.
/// An outer ring can have zero or more inner rings; firstInner()
/// returns the first in the list of inner rings.
///
class Polygonizer::Ring
{
public:
    Ring(int vertexCount, Segment* firstSegment, Ring* next) :
        firstSegment_(firstSegment),
        firstInner_(nullptr),
        next_(next),
        number_(next ? (next->number_ + 1) : 1),
        vertexCount_(vertexCount) {}

    int number() const { return number_; };
    int vertexCount() const { return vertexCount_; };
    Ring* next() const { return next_; }
    Ring* firstInner() const { return firstInner_; }
    void calculateBounds();
    #ifdef GEODESK_WITH_GEOS
    GEOSCoordSequence* createCoordSequence(GEOSContextHandle_t context);
    GEOSGeometry* createLinearRing(GEOSContextHandle_t context);
    GEOSGeometry* createPolygon(GEOSContextHandle_t context, clarisma::Arena& arena);
    #endif
    #ifdef GEODESK_WITH_OGR
    OGRLinearRing* createOgrLinearRing() const;
    OGRPolygon* createOgrPolygon() const;
    #endif

    // Sort order (used by RingMerger)
    static bool compareMinX(const Ring* a, const Ring* b) 
    {
        return a->bounds_.minX() < b->bounds_.minX();
    }

private:
    bool containsBoundsOf(const Ring* potentialInner) const
    {
        return bounds_.containsSimple(potentialInner->bounds_);
    }

    /**
     * Tests if potentialInner lies definitely within this Ring
     * (assumes containsBoundsOf() has already been checked)
     */
    bool contains(const Ring* potentialInner) const;
    PointInPolygon::Location locateCoordinate(Coordinate c) const;

    void addInner(Ring* inner)
    {
        inner->number_ = firstInner_ ? (firstInner_->number_ + 1) : 1;
        inner->next_ = firstInner_;
        firstInner_ = inner;
    }

    Segment* firstSegment_;
    Ring* firstInner_;
    Ring* next_;
    int number_;
    int vertexCount_;
    Box bounds_;

    friend class Polygonizer;
    friend class RingAssigner;
    friend class RingMerger;
    friend class RingCoordinateIterator;
};
} // namespace geodesk
