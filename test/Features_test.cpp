// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string_view>
#include <catch2/catch_test_macros.hpp>
#include <geodesk/geodesk.h>

using namespace geodesk;

struct GolFixture
{
	GolFixture() :
		world(R"(d:\geodesk\tests\world.gol)"),
		monaco(R"(d:\geodesk\tests\monaco.gol)")
	{
	}

	Features world;
	Features monaco;
};

TEST_CASE_METHOD(GolFixture, "Features")
{
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

TEST_CASE_METHOD(GolFixture, "Features2")
{
	Feature usa = world("a[boundary=administrative][admin_level=2][name='United States']").one();
	Features buildings = world("a[building]");
	Features usaBuildings = buildings(usa);
	for(int i=0; i<10; i++)
	{
		std::cout << usaBuildings.count() << " buildings" << std::endl;
	}
}


TEST_CASE_METHOD(GolFixture, "Features 3")
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

TEST_CASE_METHOD(GolFixture, "String values")
{
	std::vector<std::string> l;

	for (auto f : monaco)
	{
		for (auto tag : f.tags())
		{
			std::string s = f.toString() + ": " + static_cast<std::string>(tag.value());
			l.push_back(s);
		}
	}

	std::sort(l.begin(), l.end());

	// Write to file (UTF-8)
	std::ofstream out("d:\\geodesk\\tests\\monaco-cpp.txt",
					  std::ios::out | std::ios::trunc);
	out.imbue(std::locale::classic());
	for (const std::string& s : l)
	{
		out << s << "\n";
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

TEST_CASE_METHOD(GolFixture, "Type safety of Features")
{
	Ways ways = monaco;
	for(Feature f: ways)
	{
		REQUIRE_THROWS_AS(asNode(f), std::runtime_error);
	}
}

TEST_CASE_METHOD(GolFixture, "Empty Features")
{
	Features set = monaco("na[xyz:nonsense_tag]");
	REQUIRE(!set);
}

TEST_CASE_METHOD(GolFixture, "Lookup with empty Key")
{
	Key empty;
	TagValue v = monaco.first().value()[empty];
	REQUIRE(v == "");
}

TEST_CASE_METHOD(GolFixture, "Lookup with Key")
{
	Key highway = monaco.key("highway");
	TagValue v = monaco("w[highway]").first().value()[highway];
	REQUIRE(v != "");
}

TEST_CASE_METHOD(GolFixture, "Iterate tags of anonymous nodes")
{
	int untaggedNodeCount = 0;
	int highwayNodeCount = 0;
	for (Way street : monaco("w[highway]"))
	{
		for (Node node : street.nodes())
		{
			if (node.tags().isEmpty()) untaggedNodeCount++;
			for (Tag tag : node.tags())
			{
				if (tag.key() == "highway") highwayNodeCount++;
			}
		}
	}
	std::cout << untaggedNodeCount << " untagged nodes, "
		<< highwayNodeCount << " highway nodes.";
}

TEST_CASE("Issue 21")
{
	Features world("c:\\geodesk\\tests\\w.gol");
	Box tileBounds = Box::ofWSEN(-10, -10, 10, 10);
	Features tile = world(tileBounds);
	Features features = tile("w");
}

TEST_CASE("WayNodes")
{
	Features features("c:\\geodesk\\tests\\liguria-libero4.gol");
	uint64_t count = 0;
	for (auto street : features("w[highway]"))
	{
		for(auto node : street.nodes("n"))
		{
			std::cout << street << ": " << node << '\n';
			count++;
		}
		if (count == 10) break;
	}
	std::cout << count << " waynodes\n";
}

// TODO: Test if parent relation iterator respect types
