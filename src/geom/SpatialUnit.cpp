// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/geom/SpatialUnit.h>
#include <cstring>	// required for LengthUnit_attr

namespace geodesk {

#include "SpatialUnit_lookup.cxx"

const char* SpatialUnit::LENGTH_UNITS =
	"meters (m), kilometers (km), feet (ft), yards (yd) or miles (mi)";
const char* SpatialUnit::AREA_UNITS =
	"square_meters (m2), square_kilometers (km2), square_feet (ft2), "
	"square_yards (yd2), square_miles (mi2), hectares (hc) or acres (ac)";

int SpatialUnit::unitFromString(std::string_view unit, bool areaUnit)
{
	Unit* attr = SpatialUnit_AttrHash::lookup(unit.data(), unit.length());
	if (attr)
	{
		if ((attr->index & AREA_UNIT) == 0 || areaUnit)
		{
			return attr->index & ~AREA_UNIT;
		}
	}
	return -1;
}

// Area-unit factors represent the square root, so we can use the
//  same tables for length and area units

const double SpatialUnit::METERS_TO_UNIT[]
{
	1.0,		// meters
	0.001,      // km 
	3.280839895013123,    // feet
	1.0936132983377078,   // yards
	0.0006213711922373339,	// miles
	0.01,		// sqrt(hectares)
	0.0157195859190745,  // sqrt(acres)
};

const double SpatialUnit::UNITS_TO_METERS[] =
{
	1.0,			// meters
	1.0 / 0.001,    // km
	1.0 / 3.280839895013123,  // feet
	1.0 / 1.0936132983377078, // yards
	1.0 / 0.0006213711922373339,   // miles
	100,							// sqrt(hectares)
	63.614907234075254,  // sqrt(acres)
};

} // namespace geodesk
