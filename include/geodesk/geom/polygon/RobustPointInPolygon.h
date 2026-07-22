// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FastMemberIterator.h>
#include <geodesk/feature/WayCoordinateIterator.h>
#include <geodesk/geom/LineSegment.h>

namespace geodesk {

///
/// \cond lowlevel
///
class RobustPointInPolygon
{
public:
	static constexpr int OUTSIDE = 0;
	static constexpr int INSIDE = 1;
	static constexpr int BOUNDARY = -1;

	/// Tests the location of a point relative to a chain of
	/// coordinates: OUTSIDE (0), INSIDE (1) or BOUNDARY (-1).
	///
	/// This method works for both fully-closed individual
	/// polygons and linestrings that make up the exterior
	/// of the polygon (in which case the result of individual
	/// linestrings must be combined with `^` after checking
	/// first for BOUNDARY (-1), which short-circuits testing of
	/// further linestrings.)
	///
	/// The supplied `Iterator` must implement a `next()` method
	/// that supplies the next coordinate. A `null` `Coordinate`
	/// ends the linestring. The `Iterator` must supply at least
	/// one `Coordinate`, or the behavior of this method is undefined.
	///
	/// @param iter an object that supplies polygon Coordinates via `next()`
	/// @param pt the Coordinate to test
	///
	/// @return OUTSIDE (0) if `pt` lies outside
	///			INSIDE(1) if `pt` lies inside
	///		    BOUNDARY(-1) if `pt` lies on the boundary
	///
	/// TODO: Cannot use CoordinateSpanIterator for this,
	///  because it terminates iteration differently
	///
	template<typename Iterator>
	static int classifyBoundaryChain(Iterator& iter, Coordinate pt) noexcept
	{
		Coordinate prev = iter.next();
		int odd = 0;
		assert(!prev.isNull());
		for (;;)
		{
			Coordinate next = iter.next();
			if (next.isNull()) break;
			Coordinate lower = prev.y < next.y ? prev : next;
			Coordinate upper = prev.y < next.y ? next : prev;

			// Both boundary membership and ray crossing require the
            // point to lie within the edge's inclusive vertical range.

			if (pt.y >= lower.y && pt.y <= upper.y)
            {
	            const std::int32_t minX =
					std::min(lower.x, upper.x);
            	const std::int32_t maxX =
					std::max(lower.x, upper.x);

            	// The half-open Y interval excludes the upper endpoint from
				// the crossing count.

            	const bool crossesY = pt.y < upper.y;

            	if (pt.x < minX)
            	{
            		// The complete edge lies east of the point, so an active
            		// edge necessarily crosses the eastward ray.

            		odd ^= static_cast<int>(crossesY);
            	}
            	else if (pt.x <= maxX)
            	{
            		const std::uint64_t dy =
						static_cast<std::uint64_t>(
							static_cast<std::int64_t>(upper.y) -
							lower.y);
            		const std::uint64_t py =
						static_cast<std::uint64_t>(
							static_cast<std::int64_t>(pt.y) -
							lower.y);

            		std::uint64_t dx;
            		std::uint64_t px;
            		bool crosses;

            		if (lower.x <= upper.x)
            		{
            			dx = static_cast<std::uint64_t>(
							static_cast<std::int64_t>(upper.x) -
							lower.x);
            			px = static_cast<std::uint64_t>(
							static_cast<std::int64_t>(pt.x) -
							lower.x);

            			const std::uint64_t left = dx * py;
            			const std::uint64_t right = dy * px;

            			if (left == right) return -1;

            			crosses = left > right;
            		}
            		else
            		{
            			dx = static_cast<std::uint64_t>(
							static_cast<std::int64_t>(lower.x) -
							upper.x);
            			px = static_cast<std::uint64_t>(
							static_cast<std::int64_t>(lower.x) -
							pt.x);

            			const std::uint64_t left = dx * py;
            			const std::uint64_t right = dy * px;

            			if (left == right) return -1;

            			crosses = right > left;
            		}

            		odd ^= static_cast<int>(crossesY && crosses);
            	}
            }
			prev = next;
		}
		return odd;
	}

	static int combineResult(int a, int b) noexcept
	{
		assert(a == 0 || a == 1);
		assert(b == 0 || b == 1);
		return a ^ b;
	}

	/// Tests the location of a point relative to way that
	/// is either an area way or an inner/outer member way
	/// of a relation.
	///
	/// @param way
	/// @param pt the Coordinate to test
	///
	/// @return OUTSIDE (0) if `pt` lies outside
	///			INSIDE(1) if `pt` lies inside
	///		    BOUNDARY(-1) if `pt` lies on the boundary
	///
	///	This method does not consider the way's bbox for
	///	shortcuts; it assumes the caller has already performed
	///	any appropriate bbox shortcuts.
	///
	static int classifyBoundaryWay(const WayPtr way, Coordinate pt) noexcept
	{
		WayCoordinateIterator iter(way);
		return classifyBoundaryChain(iter, pt);
	}

	/// Checks if a full classification of a boundary way is needed
	/// based on its bounding box. If way is an area polygon, a return
	/// value of `false` means the point lies outside the polygon.
	///
	/// @param way
	/// @param pt
	///
	/// @return true if a call to classifyBoundaryWay() is required,
	///    or false if the crossing-ray does not pass through the way's
	///    bbox (so `pt` cannot be INSIDE or on the way's BOUNDARY)
	///
	static bool mustClassifyBoundaryWay(const WayPtr way, Coordinate pt) noexcept
	{
		Box bounds = way.bounds();
		return pt.y >= bounds.minY() && pt.y <= bounds.maxY();
	}

	static int classifyAreaRelation(FeatureStore* store, const RelationPtr rel, Coordinate pt)
	{
		assert(rel.isArea());
		int loc = 0;
		FastMemberIterator iter(store, rel);
			// TODO: constrain roles to outer/inner?
		for (;;)
		{
			FeaturePtr member = iter.next();
			if (member.isNull()) break;
			if (!member.isWay()) continue;
			WayPtr way(member);
			if (way.isPlaceholder()) continue;
			// TODO: do we need to check for inner/outer role?
			// Can we establish that area relations must
			// not have non-boundary ways?
			// No, admin areas often have sub-areas, which could be ways
			if (mustClassifyBoundaryWay(way, pt))
			{
				// Shortcut: We only consider the way if the horizontal
				// crossing ray passes through its bbox
				// (Note: we cannot check if the x-coordinate lies within
				// the bbox, because we need to treat the member way
				// as merely a boundary instead of a complete polygon)

				int memberLoc = classifyBoundaryWay(way, pt);
				if (memberLoc == BOUNDARY) return BOUNDARY;
				loc = combineResult(loc, memberLoc);
			}
		}
		return loc;
	}


	/*
	template<typename Iterator>
	static int isInsideNonRobust(Iterator& iter, Coordinate pt)
	{
		double px = pt.x;
		double py = pt.y;
		Coordinate prev = iter.next();
		int odd = 0;
		assert(!prev.isNull());
		for (;;)
		{
			Coordinate next = iter.next();
			if (next.isNull()) break;
			Coordinate c1 = prev.y < next.y ? prev : next;
			Coordinate c2 = prev.y < next.y ? next : prev;
			if (pt.y >= c1.y && pt.y < c2.y)
			{
				double x1 = c1.x;
				double y1 = c1.y;
				double x2 = c2.x;
				double y2 = c2.y;

				// compute edge-ray intersect x-coordinate
				double vt = (py - y1) / (y2 - y1);
				if (px < x1 + vt * (x2 - x1)) // P.x < intersect
				{
					odd ^= 1;
				}
			}
			prev = next;
		}
		return odd;
	}
	*/
};

/// \endcond

} // namespace geodesk
