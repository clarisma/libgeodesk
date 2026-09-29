// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/format/GeometryWriter.h>
#include <geodesk/geom/polygon/Polygonizer.h>
#include <geodesk/geom/polygon/Ring.h>
#include "geom/polygon/RingCoordinateIterator.h"
#include <geodesk/geom/Mercator.h>
#include <geodesk/geom/geos/GeosCoordinateIterator.h>

namespace geodesk {

void GeometryWriter::writeCoordinate(Coordinate c)
{
	if (coordStartChar_) writeByte(coordStartChar_);
	double lon = Mercator::lonFromX(c.x);
	double lat = Mercator::latFromY(c.y);
	formatDouble(latitudeFirst_ ? lat : lon, precision_);
	writeByte(coordValueSeparatorChar_);
	formatDouble(latitudeFirst_ ? lon : lat, precision_);
	if (coordEndChar_) writeByte(coordEndChar_);
}


void GeometryWriter::writeCoordinateSegment(bool isFirst, const Coordinate* p, size_t count)
{
	const Coordinate* end;
	end = p + count;
	while (p < end)
	{
		if (!isFirst) writeByte(',');  // TODO: always comma for all formats?
		isFirst = false;
		writeCoordinate(*p++);
	}
}

#ifdef GEODESK_WITH_GEOS
void GeometryWriter::writeCoordSequence(geos::geom::GeometryFactory& context, const geos::geom::CoordinateSequence& coords)
{
	GeosCoordinateIterator iter(context, coords);
    writeCoordinates(iter);
}


void GeometryWriter::writePointCoordinates(geos::geom::GeometryFactory& context, const geos::geom::Point& point)
{
    writeCoordinate(Coordinate((int32_t)std::round(point.getX()), (int32_t)std::round(point.getY())));
}


void GeometryWriter::writeLineStringCoordinates(geos::geom::GeometryFactory& context, const geos::geom::LineString& line)
{
    writeCoordSequence(context, *line.getCoordinates());
}


void GeometryWriter::writePolygonCoordinates(geos::geom::GeometryFactory& context, const geos::geom::Polygon& polygon)
{
    writeByte(coordGroupStartChar_);
    const geos::geom::LinearRing* exteriorRing = polygon.getExteriorRing();
    writeCoordSequence(context, *exteriorRing->getCoordinates());

    int numInteriorRings = polygon.getNumInteriorRing();
    for (int i = 0; i < numInteriorRings; i++)
    {
        const geos::geom::LinearRing* interiorRing = polygon.getInteriorRingN(i);
        writeByte(',');  // TODO: always comma for all formats?
        writeCoordSequence(context, *interiorRing->getCoordinates());
    }
    writeByte(coordGroupEndChar_);
}


void GeometryWriter::writeMultiPolygonCoordinates(geos::geom::GeometryFactory& context, const geos::geom::MultiPolygon& multiPolygon)
{
    writeMultiGeometryCoordinates(context, multiPolygon,
        [this](geos::geom::GeometryFactory& ctx, const geos::geom::Polygon& g)
        {
            writePolygonCoordinates(ctx, g);
        });
}

void GeometryWriter::writeGeometryCoordinates(
	geos::geom::GeometryFactory& context, geos::geom::GeometryTypeId type, const geos::geom::Geometry& geom)
{
    switch (type) 
    {
    case geos::geom::GeometryTypeId::GEOS_POINT:
        writePointCoordinates(context, dynamic_cast<const geos::geom::Point&>(geom));
        break;
    case geos::geom::GeometryTypeId::GEOS_LINESTRING:
    case geos::geom::GeometryTypeId::GEOS_LINEARRING:
        writeLineStringCoordinates(context, dynamic_cast<const geos::geom::LineString&>(geom));
        break;
    case geos::geom::GeometryTypeId::GEOS_POLYGON:
        writePolygonCoordinates(context, dynamic_cast<const geos::geom::Polygon&>(geom));
        break;
    case geos::geom::GeometryTypeId::GEOS_MULTIPOINT:
        writeMultiGeometryCoordinates(context, dynamic_cast<const geos::geom::MultiPoint&>(geom),
            [this](geos::geom::GeometryFactory& ctx, const geos::geom::Point& g)
            {
                writePointCoordinates(ctx, g);
            });
        break;
    case geos::geom::GeometryTypeId::GEOS_MULTILINESTRING:
        writeMultiGeometryCoordinates(context, dynamic_cast<const geos::geom::MultiLineString&>(geom),
            [this](geos::geom::GeometryFactory& ctx, const geos::geom::LineString& g)
            {
                writeLineStringCoordinates(ctx, g);
            });
        break;
    case geos::geom::GeometryTypeId::GEOS_MULTIPOLYGON:
        writeMultiPolygonCoordinates(context, dynamic_cast<const geos::geom::MultiPolygon&>(geom));
        break;
    }
    // (heterogeneous collections need special handling by caller)
}
#endif


void GeometryWriter::writeWayCoordinates(WayPtr way, bool group)
{
    WayCoordinateIterator iter(way);
    // TODO: Leaflet doesn't need duplicate end coordinate for polygons
    bool isFirst = true;
    if(group) writeByte(coordGroupStartChar_);
    writeByte(coordGroupStartChar_);
    for (;;)
    {
        Coordinate c = iter.next();
        if (c.isNull()) break;
        if (!isFirst) writeByte(',');  // TODO: always comma for all formats?
        isFirst = false;
        writeCoordinate(c);
    }
    writeByte(coordGroupEndChar_);
    if (group) writeByte(coordGroupEndChar_);
}



void GeometryWriter::writePolygonizedCoordinates(const Polygonizer& polygonizer)
{
    const Polygonizer::Ring* first = polygonizer.outerRings();
    assert (first);
    if (first->next()) writeByte(coordGroupStartChar_);
    const Polygonizer::Ring* ring = first;
    bool isFirst = true;
    do
    {
        if (!isFirst) writeByte(',');  // TODO: always comma for all formats?
        isFirst = false;
        writeByte(coordGroupStartChar_);
        RingCoordinateIterator iter(ring);
        writeCoordinates(iter);
        const Polygonizer::Ring* inner = ring->firstInner();
        while (inner)
        {
            writeByte(',');  // TODO: always comma for all formats?
            RingCoordinateIterator iterInner(inner);
            writeCoordinates(iterInner);
            inner = inner->next();
        }
        writeByte(coordGroupEndChar_);
        ring = ring->next();
    }
    while (ring);
    if (first->next()) writeByte(coordGroupEndChar_);
}

} // namespace geodesk
