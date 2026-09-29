// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/BufferWriter.h>
#ifdef GEODESK_WITH_GEOS
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Geometry.h>
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
	void writeCoordSequence(geos::geom::GeometryFactory& context, const geos::geom::CoordinateSequence& coords);
	void writePointCoordinates(geos::geom::GeometryFactory& context, const geos::geom::Point& point);
	void writeLineStringCoordinates(geos::geom::GeometryFactory& context, const geos::geom::LineString& line);
	void writePolygonCoordinates(geos::geom::GeometryFactory& context, const geos::geom::Polygon& polygon);
	void writeMultiPolygonCoordinates(geos::geom::GeometryFactory& context, const geos::geom::MultiPolygon& multiPolygon);
	void writeGeometryCoordinates(geos::geom::GeometryFactory& context, geos::geom::GeometryTypeId type, const geos::geom::Geometry& geom);

	template <typename MultiType, typename WriteFunc>
	void writeMultiGeometryCoordinates(
		geos::geom::GeometryFactory& context,
		const MultiType& multi,
		WriteFunc&& writeFunc)
	{
		writeByte(coordGroupStartChar_);
		std::size_t count = multi.getNumGeometries();
		for (std::size_t i = 0; i < count; i++)
		{
			if (i > 0) writeByte(',');
			// 'geom' deduces to Point&, Polygon&, or Geometry& based on MultiType
			const auto& geom = *multi.getGeometryN(i);
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
