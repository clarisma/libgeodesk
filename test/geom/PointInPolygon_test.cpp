// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <geodesk/geom/polygon/RobustPointInPolygon.h>

using namespace geodesk;

template<typename Iterator>
    static int locateIter_bad(Iterator& iter, Coordinate pt)
{
    int count = 0;
    Coordinate prev = iter.next();
    for (;;)
    {
        Coordinate next = iter.next();
        if (next.isNull()) break;

        // we normalize the vector so it always points upwards
        Coordinate start = prev.y < next.y ? prev : next;
        Coordinate end = prev.y < next.y ? next : prev;

        // TODO: (point_.y >= start.y && point_.y < end.y)
        //  and get rid of the half-counts
        if (pt.y >= start.y && pt.y <= end.y)
        {
            // TODO: this could be more efficient

            int orientation = LineSegment::orientation(start, end, pt);
            if (orientation == 0) return -1;
            count += orientation > 0 ?
                ((pt.y == start.y || pt.y == end.y) ? 1 : 2) : 0;
            // We count a ray crossing through a vertex as one-half
        }
        prev = next;
    }
    return (count & 2) >> 1;
}


class TestCoordinateIterator
{
public:
  template<typename Container>
  TestCoordinateIterator(const Container& container) :
      TestCoordinateIterator(container.data(), container.size()) {}

    TestCoordinateIterator(const Coordinate* coords, size_t count) :
        p_(coords),
        end_(coords + count)
    {
    }

    Coordinate next()
    {
        if(p_ == end_) return Coordinate();
        return *p_++;
    }

private:
    const Coordinate* p_;
    const Coordinate* end_;
};


static int locate(const std::span<const Coordinate>& coords, int32_t x, int32_t y)
{
    TestCoordinateIterator iter(coords);
    // return locateIter_bad(iter, Coordinate(x,y));
    return RobustPointInPolygon::classifyBoundaryChain(iter, Coordinate(x,y));
}

static void reverse(std::span<Coordinate> coords)
{
    Coordinate* front = coords.data();
    Coordinate* back = coords.data() + coords.size() - 1;
    while (front < back)
    {
        std::swap(*front, *back);
        front++;
        back--;
    }
}

TEST_CASE("PointInPolygon::locate with extremely long edges")
{
    // diamond that spreads to far edges of the map
    auto coords1 = std::to_array<Coordinate>(
    {
        {0, INT_MAX},
        {INT_MAX, 0},
        {0, INT_MIN},
        {INT_MIN, 0},
        {0, INT_MAX},
    });

    // aabb that span the whole map
    auto coords2 = std::to_array<Coordinate>(
    {
        {INT_MIN, INT_MAX},
        {INT_MAX, INT_MAX},
        {INT_MAX, INT_MIN},
        {INT_MIN, INT_MIN},
        {INT_MIN, INT_MAX},
    });

    for (int pass=0; pass<2; pass++)
    {
        // Test points against the diamond

        REQUIRE(locate(coords1, INT_MIN, INT_MAX) == 0);
        REQUIRE(locate(coords1, 0, INT_MAX) == -1);
        REQUIRE(locate(coords1, INT_MAX, INT_MAX) == 0);
        REQUIRE(locate(coords1, INT_MIN,0) == -1);
        REQUIRE(locate(coords1, 0,0) == 1);
        REQUIRE(locate(coords1, INT_MAX,0) == -1);
        REQUIRE(locate(coords1, INT_MIN, INT_MIN) == 0);
        REQUIRE(locate(coords1, 0, INT_MIN) == -1);
        REQUIRE(locate(coords1, INT_MAX, INT_MIN) == 0);

        REQUIRE(locate(coords1,
            INT_MAX / 2, INT_MAX - INT_MAX / 2) == -1);
        REQUIRE(locate(coords1,
            INT_MAX - INT_MAX / 2, INT_MAX / 2) == -1);
        REQUIRE(locate(coords1,
            INT_MAX / 2 - 1, INT_MAX - INT_MAX / 2) == 1);
        REQUIRE(locate(coords1,
            INT_MAX - INT_MAX / 2 - 1, INT_MAX / 2) == 1);
        REQUIRE(locate(coords1,
            INT_MAX / 2 + 1, INT_MAX - INT_MAX / 2) == 0);
        REQUIRE(locate(coords1,
            INT_MAX - INT_MAX / 2 + 1, INT_MAX / 2) == 0);

        // Test points against the box

        REQUIRE(locate(coords2, INT_MIN, INT_MIN) == -1);
        REQUIRE(locate(coords2, INT_MAX, INT_MAX) == -1);
        REQUIRE(locate(coords2, INT_MAX, INT_MIN) == -1);
        REQUIRE(locate(coords2, INT_MIN, INT_MIN) == -1);
        REQUIRE(locate(coords2, INT_MIN, 0) == -1);
        REQUIRE(locate(coords2, INT_MAX, 0) == -1);
        REQUIRE(locate(coords2, 0, INT_MIN) == -1);
        REQUIRE(locate(coords2, 0, INT_MAX) == -1);
        REQUIRE(locate(coords2, 0, 0) == 1);

        // Flip the coordinates and test again
        // because winding order is considered irrelevant

        reverse(coords1);
        reverse(coords2);
    }
}
