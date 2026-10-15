// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifdef GEODESK_WITH_GEOS

#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Geometry.h>
#include <geodesk/geom/index/MonotoneChain.h>
#include <geodesk/geom/Coordinate.h>
#include <geodesk/geom/geos/GeosCoordinateIterator.h>

namespace geodesk {

// TODO: Make this a template: Source, Iter
//  template<typename Source, typename Iter>
class CoordSequenceSlicer
{
public:
	CoordSequenceSlicer(geos::geom::GeometryFactory& context, const geos::geom::CoordinateSequence& coords);
	bool hasMore() const { return hasMore_; }
	void slice(MonotoneChain* chain, int maxVertexes);

private:
	// keep this order!
	GeosCoordinateIterator iter_;
	Coordinate first_;
	Coordinate second_;
	bool hasMore_;
};

} // namespace geodesk

#endif