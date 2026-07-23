// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <geodesk/geodesk.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <vector>

using namespace geodesk;

constexpr std::size_t LOCATION_COUNT = 1'000;
constexpr std::uint32_t RANDOM_SEED = 0x5EED'1234U;

constexpr double WEST_LONGITUDE = -10.0;
constexpr double EAST_LONGITUDE = 15.0;
constexpr double SOUTH_LATITUDE = 36.0;
constexpr double NORTH_LATITUDE = 59.0;

/// Produces a reproducible value in the range [0, 1).
double nextUnitValue(std::mt19937& generator) noexcept
{
    constexpr double ENGINE_RANGE = 4'294'967'296.0;

    return static_cast<double>(generator()) / ENGINE_RANGE;
}

/// Produces a reproducible value in the range [minimum, maximum).
double nextValue(
    std::mt19937& generator,
    double minimum,
    double maximum) noexcept
{
    return minimum +
        nextUnitValue(generator) * (maximum - minimum);
}

/// Creates reproducible Mercator-projected benchmark locations.
std::vector<geodesk::Coordinate> makeLocations()
{
    std::mt19937 generator(RANDOM_SEED);

    std::vector<geodesk::Coordinate> locations;
    locations.reserve(LOCATION_COUNT);

    for (std::size_t i = 0; i < LOCATION_COUNT; ++i)
    {
        const double lon = nextValue(
            generator,
            WEST_LONGITUDE,
            EAST_LONGITUDE);

        const double lat = nextValue(
            generator,
            SOUTH_LATITUDE,
            NORTH_LATITUDE);

        locations.emplace_back(Coordinate::ofLonLat(lon, lat));
    }

    return locations;
}

TEST_CASE("Containing queries across Western Europe")
{
    const char* golPath = "d:\\geodesk\\tests\\w2.gol"; // std::getenv("GEODESK_BENCHMARK_GOL");

    REQUIRE(golPath != nullptr);
    REQUIRE(golPath[0] != '\0');

    Features world(golPath);

    Features adminAreas = world(
        "a[boundary=administrative][admin_level<=6]");
    // Features adminAreas = world("a");

    const std::vector<Coordinate> locations =
        makeLocations();

    BENCHMARK("1,000 containing(Coordinate) queries")
    {
        std::uint64_t matchCount = 0;

        for (Coordinate xy : locations)
        {
            matchCount += adminAreas.containing(xy).count();
        }

        // std::cout << matchCount << '\n';  // TODO: remove

        return matchCount;
    };
}


TEST_CASE("All buildings in the US (incl. overseas territories)")
{
    const char* golPath = "d:\\geodesk\\tests\\w2.gol"; // std::getenv("GEODESK_BENCHMARK_GOL");

    REQUIRE(golPath != nullptr);
    REQUIRE(golPath[0] != '\0');

    Features world(golPath);
    Feature usa = world("a[boundary=administrative][admin_level=2]"
        "[name='United States']").one();
    Features buildings = world("a[building]");

    BENCHMARK("Single query (including construction)")
    {
        uint64_t count = buildings(usa).count();
        return count;
    };
}
