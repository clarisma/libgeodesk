// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/Buffer.h>
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
class CoordinateFormat
{
public:
	void precision(int precision)
	{ 
		assert(precision >= 0);
		assert(precision <= 15);
		precision_ = precision;
	}

	void write(clarisma::Buffer& out, Coordinate c, char leadChar = 0) const;
	
	template<typename Iter>
	void write(clarisma::Buffer& out, Iter& iter) const
	{
		char separatorChar = 0;
		out.writeByte(coordGroupStartChar_);
		for (int count = iter.coordinatesRemaining(); count > 0; count--)
		{
			Coordinate c = iter.next();
			write(out, c, separatorChar);
			separatorChar = ',';		// TODO: always comma for all formats?
		}
		out.writeByte(coordGroupEndChar_);
	}

	void writeWayCoordinates(clarisma::Buffer& buf, WayPtr way, bool group) const;
	void writePolygonizedCoordinates(clarisma::Buffer& out, const Polygonizer& polygonizer) const;

protected:
	uint8_t precision_ = 7;
	bool latitudeFirst_ = false;
	char coordValueSeparatorChar_ = ',';
	char coordStartChar_ = '[';
	char coordEndChar_ = ']';
	char coordGroupStartChar_ = '[';
	char coordGroupEndChar_ = ']';
};

// \endcond

} // namespace geodesk
