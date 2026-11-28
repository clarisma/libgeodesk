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
class WktFormatter : public FeatureFormatter<WktFormatter>
{
	using Buffer = clarisma::Buffer;

public:
	WktFormatter()
	{
		coordValueSeparatorChar_ = ' ';
		coordStartChar_ = 0;
		coordEndChar_ = 0;
		coordGroupStartChar_ = '(';
		coordGroupEndChar_ = ')';
	}

	void writeNodeGeometry(Buffer& out, NodePtr node) const
    {
		out.write("POINT(");
		write(out, node.xy());
		out.writeByte(')');
    }

	void writeWayGeometry(Buffer& out, WayPtr way) const
    {
		out.write(way.isArea() ? "POLYGON" : "LINESTRING");
		writeWayCoordinates(out, way, way.isArea());
    }

	void writeAreaRelationGeometry(Buffer& out, const FeatureStore* store, RelationPtr rel) const
    {
		Polygonizer polygonizer;
		polygonizer.createRings(store, rel);
		polygonizer.assignAndMergeHoles();
		const Polygonizer::Ring* ring = polygonizer.outerRings();
		int count = ring ? (ring->next() ? 2 : 1) : 0;
		out.write(count > 1 ? "MULTIPOLYGON" : "POLYGON");
		if (count == 0)
		{
			out.write(" EMPTY");
		}
		else
		{
			writePolygonizedCoordinates(out, polygonizer);
		}
    }

    void writeCollectionRelationGeometry(Buffer& out, const FeatureStore* store, RelationPtr rel) const
    {
		out.write("GEOMETRYCOLLECTION");
		if (writeMemberGeometries(out, store, rel) == 0) out.write(" EMPTY");
    }
};

// \endcond

} // namespace geodesk
