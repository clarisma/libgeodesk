// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/WayPtr.h>
#include <geodesk/feature/RelationPtr.h>
#include <clarisma/alloc/Arena.h>
#ifdef GEODESK_WITH_GEOS
#include <geos_c.h>
#endif
#ifdef GEODESK_WITH_OGR
#include <ogr_geometry.h>
#endif

namespace geodesk {

/// @brief Utility class for creating polygon rings from Relation members.
/// In OpenStreetMap, polygons with holes and multi-polygons are represented
/// as relations. Member ways with role `outer` and `inner` form the linework.
/// The Polygonizer assembles these linestrings into rings and ensures that
/// inner rings are matched with their corresponding outer rings.
///
class Polygonizer
{
public:
    /// @brief A successfully assembled ring.
    class Ring;

    Polygonizer();

    /// @brief Returns the assembled outer rings.
    /// The Ring objects form a linked list. createRings()
    /// must have been called, or the result of this method
    /// will be `nullptr`.
    ///
    /// @return a pointer the first outer ring, or `nullptr` if
    ///     no valid outer rings could be assembled.
    ///
    const Ring* outerRings() const { return outerRings_; };

    /// @brief Returns the assembled outer rings.
    /// The Ring objects form a linked list. createRings()
    /// must have been called, or the result of this method
    /// will be `nullptr`. After a subsequent call to
    /// assignAndMergeHoles() this method will always return
    /// `nullptr`, as the inner rings will have been assigned
    /// to their outer rings.
    ///
    /// @return a pointer the first outer ring, or `nullptr` if
    ///     no valid outer rings could be assembled, or if they
    ///     have been assigned to their outer rings.
    ///
    const Ring* innerRings() const { return innerRings_; };

    /// @brief Creates only the raw outer and inner rings, without assigning
    /// the inner rings to outer, and without merging inner rings
    /// whose edges touch. (This is sufficient for many operations
    /// such as area or centroid computation)
    ///
    /// @param store    the FeatureStore
    /// @param relation pointer to the stored Relation
    ///
    void createRings(FeatureStore* store, RelationPtr relation);

    /// @brief Assigns inner rings to outer, and merges any inner
    /// rings whose edges touch. The createRings() method must
    /// have been called.
    ///
    void assignAndMergeHoles();
    #ifdef GEODESK_WITH_GEOS
    GEOSGeometry* createPolygonal(GEOSContextHandle_t context);
    #endif
    #ifdef GEODESK_WITH_OGR
    OGRGeometry* createOgrPolygonal() const;
    #endif

private:
    class Segment;
    class RingBuilder;
    class RingAssigner;
    class RingMerger;

    Segment* createSegment(WayPtr way, Segment* next);
    Ring* buildRings(int segmentCount, Segment* firstSegment);

    static Ring* createRing(int vertexCount, Segment* firstSegment, 
        Ring* next, clarisma::Arena& arena);
    
    clarisma::Arena arena_;
    Ring* outerRings_;
    Ring* innerRings_;

    friend class RingCoordinateIterator;
};

} // namespace geodesk
