// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef CLARISMA_KEEP_BUFFERWRITER

#include <geodesk/format/GeoJsonWriter.h>
#include <geodesk/version.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/geom/polygon/Polygonizer.h>
#include <geodesk/geom/polygon/Ring.h>

using namespace clarisma;

namespace geodesk {

void GeoJsonWriter::writeTags(TagIterator& iter)
{
	if (pretty_)
	{
		writeConstString("{\n\t\t\t\t");
	}
	else
	{
		writeByte('{');
	}
	bool first = true;
	for(;;)
	{
		std::pair<const ShortVarString*, TagBits> tag = iter.next();
		const ShortVarString* key = tag.first;
		if(!key) break;
		TagBits value = tag.second;

		if (!first)
		{
			if (pretty_)
			{
				writeConstString(",\n\t\t\t\t");
			}
			else
			{
				writeByte(',');
			}
		}
		else
		{
			first = false;
		}
		writeByte('\"');
		writeJsonEscapedString(key->toStringView());
		if (pretty_)
		{
			writeConstString("\": ");
		}
		else
		{
			writeConstString("\":");
		}
		writeTagValue(iter.tags(), value, iter.strings());
	}
	if (pretty_)
	{
		writeConstString("\n\t\t\t}\n");
	}
	else
	{
		writeByte('}');
	}
}


void GeoJsonWriter::writeHeader()
{
	if (linewise_) return;  // No header for GeoJSONL
	if (pretty_)
	{
		writeConstString(
			"{\n"
			"\t\"type\": \"FeatureCollection\",\n"
			"\t\"generator\": \"geodesk-py/"
			GEODESK_VERSION
			"\",\n"
			"\t\"features\": [\n");
	}
	else
	{
		writeConstString(
			"{\"type\":\"FeatureCollection\",\"generator\":\"geodesk-py/"
			GEODESK_VERSION
			"\",\"features\":[");
	}
}

void GeoJsonWriter::writeFooter()
{
	if (linewise_) return;  // No footer for GeoJSONL
	if (pretty_)
	{
		writeConstString("\n\t]\n}");
	}
	else
	{
		writeConstString("]}");
	}
}

/*
void GeoJsonWriter::writeId(FeatureRef feature)
{
	if (pretty_)
	{
		writeConstString("\"id\": \"");
	}
	else
	{
		writeConstString("\"id\":\"");
	}
	writeByte("NWRX"[feature.typeCode()]);
	formatInt(feature.id());		// TODO: rename, is long in reality
	writeConstString("\",");
}
*/

void GeoJsonWriter::writeNodeGeometry(NodePtr node)
{
	if (pretty_)
	{
		writeConstString("{ \"type\": \"Point\", \"coordinates\": ");
	}
	else
	{
		writeConstString("{\"type\":\"Point\",\"coordinates\":");
	}
	writeCoordinate(node.xy());
	writeByte('}');
}



void GeoJsonWriter::writeWayGeometry(WayPtr way)
{
	if (way.isArea())
	{
		if (pretty_)
		{
			writeConstString("{ \"type\": \"Polygon\", \"coordinates\": ");
		}
		else
		{
			writeConstString("{\"type\":\"Polygon\",\"coordinates\":");
		}
	}
	else
	{
		if (pretty_)
		{
			writeConstString("{ \"type\": \"LineString\", \"coordinates\": ");
		}
		else
		{
			writeConstString("{\"type\":\"LineString\",\"coordinates\":");
		}
	}
	writeWayCoordinates(way, way.isArea());
	writeByte('}');
}

void GeoJsonWriter::writeAreaRelationGeometry(FeatureStore* store, RelationPtr relation)
{
	Polygonizer polygonizer;
	polygonizer.createRings(store, relation);
	polygonizer.assignAndMergeHoles();
	const Polygonizer::Ring* ring = polygonizer.outerRings();
	int count = ring ? (ring->next() ? 2 : 1) : 0;
	if (count > 1)
	{
		if (pretty_)
		{
			writeConstString("{ \"type\": \"MultiPolygon\", \"coordinates\": ");
		}
		else
		{
			writeConstString("{\"type\":\"MultiPolygon\",\"coordinates\":");
		}
	}
	else
	{
		if (pretty_)
		{
			writeConstString("{ \"type\": \"Polygon\", \"coordinates\": ");
		}
		else
		{
			writeConstString("{\"type\":\"Polygon\",\"coordinates\":");
		}
	}
	if (count == 0)
	{
		writeConstString("[]");
	}
	else
	{
		writePolygonizedCoordinates(polygonizer);
	}
	writeByte('}');
}


void GeoJsonWriter::writeCollectionRelationGeometry(FeatureStore* store, RelationPtr relation)
{
	if (pretty_)
	{
		writeConstString("{ \"type\": \"GeometryCollection\", \"geometries\": ");
	}
	else
	{
		writeConstString("{\"type\":\"GeometryCollection\",\"geometries\":");
	}
	if (writeMemberGeometries(store, relation) == 0)
	{
		writeConstString("[]");
	}
	writeConstString("}");
}

void GeoJsonWriter::writeFeature(FeatureStore* store, FeaturePtr feature)
{
	TagIterator tagIter(feature.tags(), store->strings());
	if (pretty_)
	{
		if (!firstFeature_) writeConstString(",\n");
		writeConstString(
			"\t\t{\n"
			"\t\t\t\"type\": \"Feature\",\n"
			"\t\t\t\"id\": ");
		writeId(store, feature);
		// TODO: bbox?
		writeConstString(",\n"
			"\t\t\t\"geometry\": ");
		writeFeatureGeometry(store, feature);
		writeConstString(
			",\n"
			"\t\t\t\"properties\": ");
		writeTags(tagIter);
		writeConstString(
			"\t\t}");
	}
	else
	{
		if (!firstFeature_) writeByte(linewise_ ? '\n' : ',');
		writeConstString("{\"type\":\"Feature\",\"id\":");
		writeId(store, feature);
		// TODO: bbox?
		writeConstString(",\"geometry\":");
		writeFeatureGeometry(store, feature);
		writeConstString(",\"properties\":");
		writeTags(tagIter);
		writeByte('}');
	}
	firstFeature_ = false;
}

void GeoJsonWriter::writeAnonymousNodeNode(Coordinate point)
{
	if (pretty_)
	{
		if (!firstFeature_) writeConstString(",\n");
		writeConstString(
			"\t\t{\n"
			"\t\t\t\"type\": \"Feature\",\n\t\t\t"
			"\t\t\t\"geometry\": "
			"{ \"type\": \"Point\", \"coordinates\": ");
		writeCoordinate(point);
		writeConstString(
			"}\n\t\t}");
	}
	else
	{
		if (!firstFeature_) writeByte(',');
		writeConstString(
			"{\"type\":\"Feature\",\"geometry\":"
			"{\"type\":\"Point\",\"coordinates\":");
		writeCoordinate(point);
		writeConstString("}}");
	}
	firstFeature_ = false;
}

} // namespace geodesk

#endif