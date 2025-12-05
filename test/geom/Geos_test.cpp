// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef GEODESK_WITH_GEOS

#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string_view>
#include <catch2/catch_test_macros.hpp>
#include <geos_c.h>
#include <geodesk/geodesk.h>

using namespace geodesk;

TEST_CASE("Intersects with GEOS geometry (#26)")
{
	GEOSContextHandle_t context = GEOS_init_r();
	if (!context)
	{
		throw std::runtime_error("Failed to initialize GEOS context");
	}

	Features france("d:\\geodesk\\tests\\france.gol");
	Feature paris = france("a[boundary=administrative][admin_level=8][name=Paris]").one();
	GEOSGeometry* geomParis = paris.toGeometry(context);
	uint64_t countByFeature = france.intersecting(paris).count();
	uint64_t countByGeom = france.intersecting(context, geomParis).count();
	REQUIRE(countByFeature > 1000);
	REQUIRE(countByFeature == countByGeom);
}

#endif

