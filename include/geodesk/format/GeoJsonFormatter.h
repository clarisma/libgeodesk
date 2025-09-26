// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "FeatureFormatter.h"
#include <clarisma/util/Json.h>
#include <geodesk/geom/polygon/Polygonizer.h>
#include <geodesk/geom/polygon/Ring.h>

namespace geodesk {

///
/// \cond lowlevel
///
class GeoJsonFormatter : public FeatureFormatter<GeoJsonFormatter>
{
public:
    using FeatureFormatter::FeatureFormatter;

	void writeNodeGeometry(clarisma::Buffer& out, NodePtr node) const
  	{
		out.write("{\"type\":\"Point\",\"coordinates\":");
  		write(out,node.xy());
		out.writeByte('}');
  	}

	void writeWayGeometry(clarisma::Buffer& out, WayPtr way) const
    {
		out.write(way.isArea() ?
			"{\"type\":\"Polygon\",\"coordinates\":" :
			"{\"type\":\"LineString\",\"coordinates\":");
		writeWayCoordinates(out, way, way.isArea());
		out.writeByte('}');
    }

	void writeAreaRelationGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const
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

	void writeCollectionRelationGeometry(clarisma::Buffer& out, FeatureStore* store, RelationPtr rel) const
    {
		out.write("{\"type\":\"GeometryCollection\",\"geometries\":");
    	if (writeMemberGeometries(out, store, rel) == 0)
		{
			out.write("[]");
		}
		out.writeByte('}');
    }

	void writeStringTagValue(clarisma::Buffer& out, const clarisma::ShortVarString* s) const
	{
		out.writeByte('\"');
		clarisma::Json::writeEscaped(out, s->toStringView());
		out.writeByte('\"');
	}

	void writeTags(clarisma::Buffer& out, TagTablePtr tags, StringTable& strings) const
    {
		FilteredTagWalker iter(tags, strings, schema_);
		char separatorChar = '{';
		while(iter.next())
        {
			out.writeByte(separatorChar);
			separatorChar = ',';
			out.writeByte('\"');
			clarisma::Json::writeEscaped(out, iter.key()->toStringView());
			out << "\":";
			writeTagValue(out, iter);
        }
		out.writeByte('}');
    }

	void writeFeature(clarisma::Buffer& out, FeatureStore* store, FeaturePtr feature)
	{
		out << "{\"type\":\"Feature\",\"id\":\"";
		writeDefaultId(out, feature);	// TODO: customizable ID
		// TODO: bbox?
		out << "\",\"geometry\":";
		writeFeatureGeometry(out, store, feature);
		out << ",\"properties\":";
		writeTags(out, feature.tags(), store->strings());
		out.writeByte('}');
	}

	// TODO: No header for GeoJSONL
	void writeHeader(clarisma::Buffer& out, const std::string_view& generator)
	{
		out << "{\"type\":\"FeatureCollection\",\"generator\":\"";
		out << generator;
		out << "\",\"features\":[";
	}

	// TODO: No footer for GeoJSONL
	void writeFooter(clarisma::Buffer& out)
	{
		out << "]}";
	}
};

// \endcond

} // namespace geodesk
