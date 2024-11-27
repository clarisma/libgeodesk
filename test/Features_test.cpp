// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <iostream>
#include <memory>
#include <string_view>
#include <catch2/catch_test_macros.hpp>
#include <geodesk/geodesk.h>

using namespace geodesk;


TEST_CASE("Features")
{
	Features world(R"(c:\geodesk\tests\wxx.gol)");
	Feature france = world("a[boundary=administrative][admin_level=2][name=France]").one();
	Feature paris = world("a[boundary=administrative][admin_level=8][name=Paris]")(france).one(); // first().value();
	std::cout << "Population of Paris: " << paris["population"] << std::endl;
	REQUIRE(paris["name"] == "Paris");
	REQUIRE(paris["population"] > 2'000'000);
	/*
	Feature usa = world("a[boundary=administrative][admin_level=2][name='United States']").one();
	Features buildings = world("a[building]");
	printf("%lld buildings in the US\n", buildings.within(usa).count());
	*/
	Ways streets = world("[highway=primary]");
	std::cout << "There are " << streets.within(paris).count() << " streets" << std::endl;
	for (Way street : streets.within(paris))
	{
		std::cout << street["name"] << std::endl;
	}
}

TEST_CASE("Features2")
{
	Features world(R"(c:\geodesk\tests\wxx.gol)");
	Feature usa = world("a[boundary=administrative][admin_level=2][name='United States']").one();
	Features buildings = world("a[building]");
	Features usaBuildings = buildings(usa);
	for(int i=0; i<10; i++)
	{
		std::cout << usaBuildings.count() << " buildings" << std::endl;
	}
}


TEST_CASE("Features 3")
{
	Features france(R"(c:\geodesk\tests\frxx.gol)");
	Feature paris = france("a[boundary=administrative][admin_level=8][name=Paris]").one();
	Features museums = france("na[tourism=museum]");
	Features subwayStops = france("n[railway=station][station=subway]");
	for(auto museum: museums(paris))
	{
		std::cout << museum["name"] << std::endl;
		for(auto stop: subwayStops.maxMetersFrom(500, museum.centroid()))
		{
			std::cout << "- " << stop["name"] << std::endl;
		}
	}
}


/*

// This does not compile (deliberate error:
// attempt to assign Way to Node)

TEST_CASE("Type safety of Features")
{
	Features world(R"(c:\geodesk\tests\monaco.gol)");
	Ways ways = world;
	for(Node n: ways)
	{
		std::cout << n.xy() << std::endl;
	}
}
*/


static Node asNode(Feature f)
{
	return f;
}

TEST_CASE("Type safety of Features")
{
	Features world(R"(c:\geodesk\tests\mcxx.gol)");
	Ways ways = world;
	for(Feature f: ways)
	{
		REQUIRE_THROWS_AS(asNode(f), std::runtime_error);
	}
}


// TODO: Test if parent relation iterator respect types
