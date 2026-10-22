// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifdef GEODESK_WITH_GEOS

#include <cmath>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Geometry.h>
#include <geodesk/geom/Coordinate.h>

namespace geodesk {

class GeosCoordinateIterator
{
public:
	GeosCoordinateIterator(geos::geom::GeometryFactory& context, const geos::geom::CoordinateSequence& coords) :
		context_(context),
		coords_(coords),
		coordCount_(0),
		current_(0)
	{
		coordCount_ = coords.size();
	}

	int coordinatesRemaining() const { return coordCount_ - current_; }
	Coordinate next()
	{
		geos::geom::CoordinateXY coords = coords_.getAt(current_++);
		return Coordinate((int32_t)std::round(coords.x), (int32_t)std::round(coords.y));
	}

private:
	geos::geom::GeometryFactory& context_;
	const geos::geom::CoordinateSequence& coords_;
	unsigned int coordCount_;
	unsigned int current_;
};



} // namespace geodesk

#endif