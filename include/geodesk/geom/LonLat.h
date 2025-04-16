// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/geom/Coordinate.h>
#include <clarisma/text/Format.h>

namespace geodesk {

/// @brief A WGS-84 coordinate pair.
///
class LonLat
{
public:
	LonLat(Coordinate c) :
		lon(Mercator::lonFromX(c.x)),
		lat(Mercator::latFromY(c.y)) {}

	char* format(char* buf, int prec = 7) const
	{
        char* p = clarisma::Format::formatDouble(buf, lon, prec);
        *p++ = ',';
		return clarisma::Format::formatDouble(buf, lat, prec);
	}

	std::string toString() const
	{
		char buf[32];
		format(buf);
		return std::string(buf);
	}

	double lon;
	double lat;
};

template<typename Stream>
Stream& operator<<(Stream& out, const LonLat& lonlat)
{
	char buf[32];
	lonlat.format(buf);
	out.write(buf, strlen(buf));
	return out;
}


} // namespace geodesk
