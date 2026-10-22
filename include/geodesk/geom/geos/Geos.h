// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifdef GEODESK_WITH_GEOS

#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Geometry.h>
#include <geodesk/geom/Box.h>

namespace geodesk {

class Geos
{
public:
	static Box getEnvelope(geos::geom::GeometryFactory& context, const geos::geom::Geometry& geom)
	{
		return Box(geom.getEnvelopeInternal());
	}

	static bool centroid(geos::geom::GeometryFactory& context,
		const geos::geom::Geometry& geom, Coordinate* centroid);

	static double distanceMeters(geos::geom::GeometryFactory& context,
		const geos::geom::Geometry& geom1, const geos::geom::Geometry& geom2);
};

} // namespace geodesk

#endif
