#pragma once

#include <geodesk/filter/Filter.h>
#include <geodesk/feature/FeaturePtr.h>

namespace geodesk {

///
/// \cond lowlevel
///
template <typename Predicate>
class GEODESK_API PredicateFilter : public Filter
{
public:
	explicit PredicateFilter(Predicate predicate) :
		predicate_(predicate)
	{
	}

	bool accept(const FeatureStore* store, FeaturePtr ptr, FastFilterHint fast) const override
	{
		geodesk::Feature feature(store, FeaturePtr(ptr.ptr()));
		return predicate_(feature);
    }

protected:
	Predicate predicate_;
};

// \endcond



} // namespace geodesk
