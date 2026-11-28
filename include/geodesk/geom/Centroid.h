// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/geom/Coordinate.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/feature/RelationPtr.h>

namespace geodesk {

/// Classes and functions for calculating a feature's centroid
///
/// @ingroup lowlevel
/// \cond lowlevel
///
class GEODESK_API Centroid
{
public:
	static Coordinate ofWay(WayPtr way);
	static Coordinate ofRelation(const FeatureStore* store, RelationPtr relation);
	static Coordinate ofFeature(const FeatureStore* store, FeaturePtr feature);

private:
	class Areal
	{
	public:
		Areal() : areaSum_(0), areaCentroidX_(0), areaCentroidY_(0) {}

		// The shoelace sums are computed relative to the ring's first vertex.
		// Using absolute coordinates (up to 2^31) makes products like x1 * y2
		// reach ~2^62, losing precision in a double; for small polygons, the
		// centroid can shift by many meters, often to outside the polygon
		// (see https://github.com/clarisma/geodesk-py/issues/102).
		// The ring's moments are translated back to absolute coordinates
		// before they are accumulated.
		//
		// Shells and holes may have either winding order: each ring is
		// normalized to positive area, then shells are added, holes subtracted.

		template<typename Iter>
		void addRing(Iter& iter, bool isShell)
		{
			double ringSum = 0;
			double ringCentroidX = 0;
			double ringCentroidY = 0;

			Coordinate first = iter.next();
			double x1 = 0;
			double y1 = 0;
			for (int count = iter.coordinatesRemaining(); count > 0; count--)
			{
				Coordinate c = iter.next();
				double x2 = static_cast<double>(c.x) - first.x;
				double y2 = static_cast<double>(c.y) - first.y;
				double a = x1 * y2 - x2 * y1;
				ringSum += a;
				ringCentroidX += (x1 + x2) * a;
				ringCentroidY += (y1 + y2) * a;
				x1 = x2;
				y1 = y2;
			}
			double sign = (isShell ? 1.0 : -1.0) * (ringSum >= 0 ? 1.0 : -1.0);
			areaSum_ += ringSum * sign;
			areaCentroidX_ += (ringCentroidX + 3.0 * ringSum * first.x) * sign;
			areaCentroidY_ += (ringCentroidY + 3.0 * ringSum * first.y) * sign;
		}

		void addAreaRelation(const FeatureStore* store, RelationPtr relation);
		bool isEmpty() const { return areaSum_ == 0; }
		Coordinate centroid() const
		{
			return Coordinate(
				static_cast<int32_t>(round(areaCentroidX_ / (3.0 * areaSum_))),
				static_cast<int32_t>(round(areaCentroidY_ / (3.0 * areaSum_))));
		}

	private:
		double areaSum_;
		double areaCentroidX_;
		double areaCentroidY_;
	};

	class Lineal
	{
	public:
		Lineal() : totalLength_(0), lineCentroidX_(0), lineCentroidY_(0) {}

		void addLineSegments(WayPtr way);

		bool isEmpty() const { return totalLength_ == 0; }
		Coordinate centroid() const
		{
			return Coordinate(
				static_cast<int32_t>(round(lineCentroidX_ / (totalLength_ * 2.0))),
				static_cast<int32_t>(round(lineCentroidY_ / (totalLength_ * 2.0))));
		}

	private:
		double totalLength_;
		double lineCentroidX_;
		double lineCentroidY_;
	};

	class Puntal
	{
	public:
		Puntal() : pointCount_(0), pointCentroidX_(0), pointCentroidY_(0) {}

		void addPoint(Coordinate point);
		bool isEmpty() const { return pointCount_ == 0; }
		Coordinate centroid() const
		{
			return Coordinate(
				static_cast<int32_t>(round(pointCentroidX_ / pointCount_)),
				static_cast<int32_t>(round(pointCentroidY_ / pointCount_)));
		}

	private:
		double pointCentroidX_;
		double pointCentroidY_;
		size_t pointCount_;
	};

private:
	void addWay(WayPtr way);
	void addRelation(const FeatureStore* store, RelationPtr rel, RecursionGuard& guard);
	
	Areal areal_;
	Lineal lineal_;
	Puntal puntal_;
};

// \endcond



} // namespace geodesk
