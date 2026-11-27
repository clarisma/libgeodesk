#pragma once

#include <geodesk/filter/Filter.h>
#include <geodesk/geom/Area.h>

namespace geodesk {

class AreaFilter : public Filter
{
public:
	AreaFilter(double minArea, double maxArea) :
		Filter(0, minArea > 0 ? FeatureTypes::AREAS : FeatureTypes::ALL),
		minArea_(minArea),
		maxArea_(maxArea)
	{
	}

	bool accept(const FeatureStore* store, FeaturePtr feature, FastFilterHint fast) const override
	{
		double a;
		if (feature.isArea())
		{
			if (feature.isWay())
			{
				a = Area::ofWay(WayPtr(feature));
			}
			else
			{
				assert(feature.isRelation());
				a = Area::ofRelation(store, RelationPtr(feature));
			}
		}
		else
		{
			a = 0;
		}
		return a >= minArea_ && a <= maxArea_;
	}

protected:
	double minArea_;
	double maxArea_;
};



} // namespace geodesk
