// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/geom/polygon/Ring.h>
#include <geodesk/geom/polygon/RobustPointInPolygon.h>
#include "Segment.h"

namespace geodesk {

#ifdef GEODESK_WITH_GEOS
GEOSCoordSequence* Polygonizer::Ring::createCoordSequence(GEOSContextHandle_t context)
{
    GEOSCoordSequence* coordSeq = GEOSCoordSeq_create_r(context, vertexCount_, 2);
    if (coordSeq)
    {
        Segment* seg = firstSegment_;
        Coordinate first = seg->backward ? seg->coords[seg->vertexCount - 1] : seg->coords[0];
        GEOSCoordSeq_setXY_r(context, coordSeq, 0, first.x, first.y);
        int pos = 1;
        do
        {
            seg->copyTo(context, coordSeq, pos);
            pos += seg->vertexCount - 1;
            seg = seg->next;
        }
        while (seg);
        assert(pos == vertexCount_);
    }
    return coordSeq;
}

GEOSGeometry* Polygonizer::Ring::createLinearRing(GEOSContextHandle_t context)
{
    GEOSCoordSequence* seq = createCoordSequence(context);
    return GEOSGeom_createLinearRing_r(context, seq);
}

// TODO: error handling, check for null returns (C API does not throw)
GEOSGeometry* Polygonizer::Ring::createPolygon(GEOSContextHandle_t context, clarisma::Arena& arena)
{
    GEOSGeometry** holes;
    int holeCount;
    if (firstInner_)
    {
        holeCount = firstInner_->number_;
        holes = arena.allocArray<GEOSGeometry*>(holeCount);
        Ring* inner = firstInner_;
        for (int i = 0; i < holeCount; i++)
        {
            holes[i] = inner->createLinearRing(context);
            inner = inner->next();
        }
    }
    else
    {
        holeCount = 0;
        holes = nullptr;
    }

    GEOSGeometry* shell = createLinearRing(context);
    return GEOSGeom_createPolygon_r(context, shell, holes, holeCount);
}
#endif

#ifdef GEODESK_WITH_OGR

OGRLinearRing* Polygonizer::Ring::createOgrLinearRing() const
{
    OGRLinearRing* ring = new OGRLinearRing();
    ring->setNumPoints(vertexCount_);
    Segment* seg = firstSegment_;
    Coordinate first = seg->backward ? seg->coords[seg->vertexCount - 1] : seg->coords[0];
    ring->setPoint(0, first.lon(), first.lat());
    int pos = 1;
    do
    {
        seg->copyTo(ring, pos);
        pos += seg->vertexCount - 1;
        seg = seg->next;
    }
    while (seg);
    assert(pos == vertexCount_);
    return ring;
}

OGRPolygon* Polygonizer::Ring::createOgrPolygon() const
{
    OGRPolygon* polygon = new OGRPolygon();
    polygon->addRingDirectly(createOgrLinearRing());
    Ring* inner = firstInner_;
    while (inner)
    {
        polygon->addRingDirectly(inner->createOgrLinearRing());
        inner = inner->next();
    }
    return polygon;
}
#endif

void Polygonizer::Ring::calculateBounds()
{
    Segment* p = firstSegment_;
    do
    {
        bounds_.expandToIncludeSimple(p->bounds());
        p = p->next;
    }
    while (p);
}


int Polygonizer::Ring::locateCoordinate(Coordinate c) const
{
    int result = 0;
    const Segment* seg = firstSegment_;
    do
    {
        if (RobustPointInPolygon::mustClassifyBoundaryWay(seg->way, c))
        {
            int wayResult = RobustPointInPolygon::classifyBoundaryWay(seg->way, c);
            if (wayResult == RobustPointInPolygon::BOUNDARY)
            {
                return RobustPointInPolygon::BOUNDARY;
            }
            result = RobustPointInPolygon::combineResult(result, wayResult);
        }
        seg = seg->next;
    }
    while (seg);
    assert(result == RobustPointInPolygon::INSIDE ||
        result == RobustPointInPolygon::OUTSIDE);
    return result;
}


bool Polygonizer::Ring::contains(const Ring* potentialInner) const
{
    // An inner ring may touch an outer in one point, which doesn't necessarily
    // mean that it lies within the outer. Therefore, if the first point checked
    // lies on the boundary, we check a second point

    Coordinate* coords = potentialInner->firstSegment_->coords;
    int loc = locateCoordinate(coords[0]);
    if (loc == RobustPointInPolygon::BOUNDARY) [[unlikely]]
    {
        loc = locateCoordinate(coords[1]);
    }
    return loc == RobustPointInPolygon::INSIDE;
}

} // namespace geodesk
