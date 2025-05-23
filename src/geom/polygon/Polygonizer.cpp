// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/geom/polygon/Polygonizer.h>
#include "Ring.h"
#include "RingBuilder.h"
#include "RingAssigner.h"
#include "RingMerger.h"
#include "Segment.h"
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/MemberIterator.h>

namespace geodesk {

Polygonizer::Polygonizer() :
	outerRings_(nullptr),
	innerRings_(nullptr)
{
}


// TODO: Use a separate path for areas (which be definition don't require assembling)?
// This way, no need to test for flag
Polygonizer::Segment* Polygonizer::createSegment(WayPtr way, Segment* next)
{
	WayCoordinateIterator iter(way);
	int vertexCount = iter.coordinatesRemaining();
    Segment* seg = arena_.allocWithExplicitSize<Segment>(Segment::sizeWithVertexCount(vertexCount));
	seg->next = next;
	seg->way = way;
	seg->status = Segment::SEGMENT_UNASSIGNED;
	seg->backward = false;
	seg->vertexCount = vertexCount;
	Coordinate* p = seg->coords;
    Coordinate* end = p + vertexCount;
	do
	{
		*p++ = iter.next();
	} 
	while (p < end);
	return seg;
}


void Polygonizer::createRings(FeatureStore* store, RelationPtr relation)
{
    Segment* outerSegments = nullptr;
    Segment* innerSegments = nullptr;
    int outerSegmentCount = 0;
    int innerSegmentCount = 0;

    DataPtr pMembers = relation.bodyptr();
    // TODO: empty relations?
    // TODO: deal with missing tiles
    // TODO: use FastMemberIterator -- no, need role
    //  But careful, MemberIterator is not threadsafe!
    //  (4/3/24: made it threadsafe by only creating the Pyhon role string object
    //  on demand; since we no longer use the Python object, MI is safe)
    MemberIterator iter(store, pMembers, FeatureTypes::WAYS, store->borrowAllMatcher(), nullptr);
    for (;;)
    {
        WayPtr way(iter.next());
        if (way.isNull()) break;
        if (way.isPlaceholder()) continue;

        /*
        LOG("Creating segment for way/%ld", way.id());
        if (way.id() == 894879603)
        {
            LOG("STOP");
        }
        */

        // TODO: Use more efficient check for inner/outer role
        // based on global string code; however, this would require
        // change to the GOL spec to ensure that "inner" and "outer"
        // are always global strings (may not be the case for very small
        // datasets where their string count falls below the minimum)

        std::string_view role = iter.currentRole();
        if (role == "outer")
        {
            outerSegments = createSegment(way, outerSegments);
            outerSegmentCount++;
        }
        else if (role == "inner")
        {
            innerSegments = createSegment(way, innerSegments);
            innerSegmentCount++;
        }
    }
    if (outerSegmentCount > 0)
    {
        outerRings_ = buildRings(outerSegmentCount, outerSegments);
    }
    if (innerSegmentCount > 0)
    {
        innerRings_ = buildRings(innerSegmentCount, innerSegments);
    }
}


Polygonizer::Ring* Polygonizer::buildRings(int segmentCount, Segment* firstSegment)
{
    assert(segmentCount > 0);
    if (segmentCount == 1)
    {
        if (firstSegment->isClosed())
        {
            firstSegment->status = Segment::SEGMENT_ASSIGNED;
            return createRing(firstSegment->vertexCount, firstSegment, nullptr, arena_);
        }
        firstSegment->status = Segment::SEGMENT_DANGLING;
        return nullptr;
    }

    RingBuilder ringBuilder(segmentCount, firstSegment, arena_);
    return ringBuilder.build();
}


Polygonizer::Ring* Polygonizer::createRing(int vertexCount, Segment* firstSegment,
    Ring* next, clarisma::Arena& arena)
{
    Ring* ring = arena.alloc<Ring>();
    new (ring) Ring(vertexCount, firstSegment, next);
    return ring;
}


void Polygonizer::assignAndMergeHoles()
{
    if (outerRings_ == nullptr) return; // nothing to do
    if (innerRings_ == nullptr) return; // no inner rings
    if (outerRings_->next() == nullptr)
    {
        // only a single outer ring: assign all inners to it
        // (We don't validate whether these rings actually lie inside)
        outerRings_->firstInner_ = innerRings_;
    }
    else
    {
        RingAssigner::assignRings(outerRings_, innerRings_, arena_);
    }
    innerRings_ = nullptr;  

    // Check if any of the rings contain holes with touching edges
    Ring* ring = outerRings_;
    do
    {
        if (ring->firstInner_ && ring->firstInner_->next())
        {
            RingMerger merger(arena_);  // single-use, does not reset itself
            ring->firstInner_ = merger.mergeRings(ring->firstInner_);
        }
        ring = ring->next();
    }
    while (ring);
}

#ifdef GEODESK_WITH_GEOS
GEOSGeometry* Polygonizer::createPolygonal(GEOSContextHandle_t context) 
{
    if (outerRings_ == nullptr)
    {
        return GEOSGeom_createEmptyPolygon();
    }

    int ringCount = 0;
    Ring* ring = outerRings_;
    do
    {
        ringCount++;
        ring = ring->next();
    }
    while (ring);

    if(ringCount == 1) return outerRings_->createPolygon(context, arena_);
    
    GEOSGeometry** polygons = arena_.allocArray<GEOSGeometry*>(ringCount);
    ring = outerRings_;
    for (int i = 0; i < ringCount; i++)
    {
        polygons[i] = ring->createPolygon(context, arena_);
        ring = ring->next();
    }
    return GEOSGeom_createCollection_r(context, GEOS_MULTIPOLYGON, polygons, ringCount);
}
#endif

} // namespace geodesk
