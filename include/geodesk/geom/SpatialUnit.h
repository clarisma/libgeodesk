// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <string_view>

namespace geodesk {

/// \cond lowlevel

class SpatialUnit
{
public:
	enum
	{
		METERS,
		KILOMETERS,
		FEET,
		YARDS,
		MILES,
		HECTARES,
		ACRES
	};

	static constexpr int AREA_UNIT = 128;

	static double fromMeters(double meters, int unit)
	{
		assert(unit >= METERS && unit <= ACRES);
		return meters * METERS_TO_UNIT[unit];
	}

	static double toMeters(double units, int unit)
	{
		assert(unit >= METERS && unit <= ACRES);
		return units * UNITS_TO_METERS[unit];
	}

	/**
	 * Returns the length unit based on the given string,
	 * or -1 if it does not represent a valid unit type.
	 */
	static int unitFromString(std::string_view unit, bool areaUnit);

	static const double METERS_TO_UNIT[];
	static const double UNITS_TO_METERS[];

	static const char* LENGTH_UNITS;
	static const char* AREA_UNITS;
};

/// \endcond
} // namespace geodesk
