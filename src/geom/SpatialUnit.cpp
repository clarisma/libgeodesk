// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/geom/SpatialUnit.h>
#include <cstring>	// required for LengthUnit_attr

#include "SpatialUnit_lookup.cxx"

namespace geodesk {

// const char* LengthUnit::VALID_UNITS = "meters (m), kilometers (km), feet (ft), yards (yd) or miles (mi)";

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

const double SpatialUnit::METERS_TO_UNIT[]
{
	1.0,		// meters
	0.001,      // km 
	3.28084,    // feet
	1.093613,   // yards
	0.0006213711922373339,	// miles
	0.01,		// hectares
	0.0157195859190745016,  // acres
};

const double SpatialUnit::UNITS_TO_METERS[] =
{
	1.0,			// meters
	1.0 / 0.001,    // km
	1.0 / 3.28084,  // feet
	1.0 / 1.093613, // yards
	1.0 / 0.0006213711922373339,   // miles
	100,							// hectares
	63.614907234075253346,  // acres
};

} // namespace geodesk
