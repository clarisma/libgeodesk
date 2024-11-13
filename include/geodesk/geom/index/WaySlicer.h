// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/WayCoordinateIterator.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/geom/index/MonotoneChain.h>
#include <geodesk/geom/Coordinate.h>

namespace geodesk {

// TODO: Make this a template: Source, Iter
//  template<typename Source, typename Iter>
class WaySlicer
{
public:
	explicit WaySlicer(WayPtr way);
	bool hasMore() const { return hasMore_; }
	void slice(MonotoneChain* chain, int maxVertexes);

private:
	// keep this order!
	WayCoordinateIterator iter_;
	Coordinate first_;
	Coordinate second_;
	bool hasMore_;
};

} // namespace geodesk
