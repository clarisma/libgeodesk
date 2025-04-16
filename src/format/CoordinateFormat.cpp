// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/format/CoordinateFormat.h>
#include <clarisma/text/Format.h>
#include <geodesk/geom/polygon/Polygonizer.h>
#include "geom/polygon/Ring.h"
#include "geom/polygon/RingCoordinateIterator.h"
#include <geodesk/geom/Mercator.h>
#include <geodesk/geom/geos/GeosCoordinateIterator.h>

namespace geodesk {

using namespace clarisma;
using namespace clarisma::Format;

void CoordinateFormat::write(Buffer& out, Coordinate c, char leadChar)
{
    char buf[64];
    char* p = buf;
    *p = leadChar;
    p += (leadChar != 0);
    *p = coordStartChar_;
	p += (coordStartChar_ != 0);
	double lon = Mercator::lonFromX(c.x);
	double lat = Mercator::latFromY(c.y);
	p = formatDouble(p, latitudeFirst_ ? lat : lon, precision_);
	*p++ = coordValueSeparatorChar_;
	p = formatDouble(p, latitudeFirst_ ? lon : lat, precision_);
	*p = coordEndChar_;
    p += (coordEndChar_ != 0);
    out.write(buf, p - buf);
}


void CoordinateFormat::writeWayCoordinates(clarisma::Buffer& out, WayPtr way, bool group)
{
    WayCoordinateIterator iter(way);
    // TODO: Leaflet doesn't need duplicate end coordinate for polygons
    if(group) out.writeByte(coordGroupStartChar_);
    out.writeByte(coordGroupStartChar_);
    char separatorChar = 0;
    for (;;)
    {
        Coordinate c = iter.next();
        if (c.isNull()) break;
        write(out, c, separatorChar);
        separatorChar = ',';		// TODO: always comma for all formats?
    }
    out.writeByte(coordGroupEndChar_);
    if (group) out.writeByte(coordGroupEndChar_);
}



void CoordinateFormat::writePolygonizedCoordinates(
    clarisma::Buffer& out, const Polygonizer& polygonizer)
{
    const Polygonizer::Ring* first = polygonizer.outerRings();
    assert (first);
    if (first->next()) out.writeByte(coordGroupStartChar_);
    const Polygonizer::Ring* ring = first;
    bool isFirst = true;
    do
    {
        if (!isFirst) out.writeByte(',');  // TODO: always comma for all formats?
        isFirst = false;
        out.writeByte(coordGroupStartChar_);
        RingCoordinateIterator iter(ring);
        write(out, iter);
        const Polygonizer::Ring* inner = ring->firstInner();
        while (inner)
        {
            out.writeByte(',');  // TODO: always comma for all formats?
            RingCoordinateIterator iterInner(inner);
            write(out, iterInner);
            inner = inner->next();
        }
        out.writeByte(coordGroupEndChar_);
        ring = ring->next();
    }
    while (ring);
    if (first->next()) out.writeByte(coordGroupEndChar_);
}

} // namespace geodesk
