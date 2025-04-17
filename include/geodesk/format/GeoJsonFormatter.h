// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "FeatureFormatter.h"
#include <geodesk/geom/polygon/Polygonizer.h>
#include <geodesk/geom/polygon/Ring.h>

namespace geodesk {

///
/// \cond lowlevel
///
class GeoJsonFormatter : public FeatureFormatter<GeoJsonFormatter>
{
public:
	void writeGeometry(clarisma::Buffer& out, NodePtr node) const
  	{
		out.write("{\"type\":\"Point\",\"coordinates\":");
  		write(out,node.xy());
		out.writeByte('}');
  	}

	void writeGeometry(clarisma::Buffer& out, WayPtr way) const
    {
		out.write(way.isArea() ?
			"{\"type\":\"Polygon\",\"coordinates\":" :
			"{\"type\":\"LineString\",\"coordinates\":");
		writeWayCoordinates(out, way, way.isArea());
		out.writeByte('}');
    }

	void writeAreaGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const
    {
		Polygonizer polygonizer;
		polygonizer.createRings(store, rel);
		polygonizer.assignAndMergeHoles();
		const Polygonizer::Ring* ring = polygonizer.outerRings();
		int count = ring ? (ring->next() ? 2 : 1) : 0;
        out.write(count > 1 ?
			"{\"type\":\"MultiPolygon\",\"coordinates\":" :
			"{\"type\":\"Polygon\",\"coordinates\":");
		if (count == 0)
		{
			out.write("[]");
		}
		else
		{
			writePolygonizedCoordinates(out, polygonizer);
		}
		out.writeByte('}');
    }

	void writeCollectionGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const
    {
		out.write("{\"type\":\"GeometryCollection\",\"geometries\":");
    	if (writeMemberGeometries(out, store, rel) == 0)
		{
			out.write("[]");
		}
		out.writeByte('}');
    }
};

// \endcond

} // namespace geodesk
