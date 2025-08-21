// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef CLARISMA_KEEP_BUFFERWRITER

#pragma once

#include <clarisma/util/BufferWriter.h>
#ifdef GEODESK_WITH_GEOS
#include <geos_c.h>
#endif
#include <geodesk/feature/WayPtr.h>
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/geom/Coordinate.h>
#include <functional>

namespace geodesk {

class Polygonizer;

///
/// \cond lowlevel
///
class GeometryWriter : public clarisma::BufferWriter
{
public:
	explicit GeometryWriter(clarisma::Buffer* buf) : BufferWriter(buf) {}
	
	void precision(int precision) 
	{ 
		assert(precision >= 0);
		assert(precision <= 15);
		precision_ = precision;
	}

protected:
	void writeCoordinate(Coordinate c);
	
	template<typename Iter>
	void writeCoordinates(Iter& iter)
	{
		bool isFirst = true;
		writeByte(coordGroupStartChar_);
		for (int count = iter.coordinatesRemaining(); count > 0; count--)
		{
			Coordinate c = iter.next();
			if (!isFirst) writeByte(',');  // TODO: always comma for all formats?
			isFirst = false;
			writeCoordinate(c);
		}
		writeByte(coordGroupEndChar_);
	}

	// ==== GEOS Geometries ====

	void writeCoordinateSegment(bool isFirst, const Coordinate* coords, size_t count);
	#ifdef GEODESK_WITH_GEOS
	void writeCoordSequence(GEOSContextHandle_t context, const GEOSCoordSequence* coords);
	void writePointCoordinates(GEOSContextHandle_t context, const GEOSGeometry* point);
	void writeLineStringCoordinates(GEOSContextHandle_t context, const GEOSGeometry* line);
	void writePolygonCoordinates(GEOSContextHandle_t context, const GEOSGeometry* polygon);
	void writeMultiPolygonCoordinates(GEOSContextHandle_t context, const GEOSGeometry* multiPolygon);
	void writeGeometryCoordinates(GEOSContextHandle_t context, int type, const GEOSGeometry* geom);

	void writeMultiGeometryCoordinates(
		GEOSContextHandle_t context, const GEOSGeometry* multi, 
		std::function<void(GEOSContextHandle_t, const GEOSGeometry*)> writeFunc)
	{
		writeByte(coordGroupStartChar_);
		int count = GEOSGetNumGeometries_r(context, multi);
		for (int i = 0; i < count; i++) 
		{
			if(i > 0) writeByte(',');
			const GEOSGeometry* geom = GEOSGetGeometryN_r(context, multi, i);
			writeFunc(context, geom);
		}
		writeByte(coordGroupEndChar_);
	}
	#endif

	// ==== Feature Geometries ====

	void writeWayCoordinates(WayPtr way, bool group);
	void writePolygonizedCoordinates(const Polygonizer& polygonizer);

	int precision_ = 7;
	bool latitudeFirst_ = false;
	char coordValueSeparatorChar_ = ',';
	char coordStartChar_ = '[';
	char coordEndChar_ = ']';
	char coordGroupStartChar_ = '[';
	char coordGroupEndChar_ = ']';
};

// \endcond

} // namespace geodesk

#endif