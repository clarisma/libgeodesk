// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#ifdef GEODESK_WITH_GEOS

#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string_view>
#include <catch2/catch_test_macros.hpp>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Geometry.h>
#include <geodesk/geodesk.h>
#include <geodesk/feature/QueryException.h>
#include <geodesk/filter/Filters.h>
#include <geodesk/geom/geos/Geos.h>

using namespace geodesk;

TEST_CASE("Intersects with GEOS geometry (#26)")
{
	geos::geom::GeometryFactory::Ptr factory = geos::geom::GeometryFactory::create();

	Features france("d:\\geodesk\\tests\\france.gol");
	Feature paris = france("a[boundary=administrative][admin_level=8][name=Paris]").one();
	std::unique_ptr<geos::geom::Geometry> geomParis = paris.toGeometry(*factory);
	uint64_t countByFeature = france.intersecting(paris).count();
	uint64_t countByGeom = france.intersecting(*factory, *geomParis).count();
	REQUIRE(countByFeature > 1000);
	REQUIRE(countByFeature == countByGeom);
}

TEST_CASE("Empty GEOS geometries are rejected by spatial filters")
{
	geos::geom::GeometryFactory::Ptr factory = geos::geom::GeometryFactory::create();
	auto point = factory->createPoint();
	auto line = factory->createLineString();

	REQUIRE_THROWS_AS(Filters::intersecting(*factory, *point), QueryException);
	REQUIRE_THROWS_AS(Filters::intersecting(*factory, *line), QueryException);
}

TEST_CASE("GEOS helpers handle empty geometries")
{
	geos::geom::GeometryFactory::Ptr factory = geos::geom::GeometryFactory::create();
	auto empty = factory->createPoint();
	auto point = factory->createPoint(geos::geom::Coordinate(1, 2));
	Coordinate centroid;

	REQUIRE_FALSE(Geos::centroid(*factory, *empty, &centroid));
	REQUIRE(Geos::distanceMeters(*factory, *empty, *point) == -1);
}

#endif
