// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifdef GEODESK_WITH_GEOS

#include <geos_c.h>
#include <geos/geom/Geometry.h>
#include <geodesk/geom/Box.h>

namespace geodesk {

class Geos
{
public:
	static Box getEnvelope(GEOSContextHandle_t context, const GEOSGeometry* geom)
	{
		return Box(((geos::geom::Geometry*)geom)->getEnvelopeInternal());
	}

	static bool centroid(GEOSContextHandle_t context,
		const GEOSGeometry* geom, Coordinate* centroid);
};

} // namespace geodesk

#endif
