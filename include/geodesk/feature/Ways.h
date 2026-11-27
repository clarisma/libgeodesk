// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "FeaturesBase.h"
#include "Relations.h"

namespace geodesk {

/// \cond
/// @brief A collection containing only Way features.
///
class Ways : public FeaturesBase<Way>
{
public:
	using FeaturesBase::FeaturesBase;
	Ways(const FeaturesBase<Feature>& other) :
		FeaturesBase(other.withTypes(FeatureTypes::WAYS)) {}
	Ways(const FeaturesBase<Node>& other) : FeaturesBase(other.getEmpty()) {}
	Ways(const FeaturesBase<Way>& other) : FeaturesBase(other) {}
	Ways(const FeaturesBase<Relation>& other) : FeaturesBase(other.getEmpty()) {}

	template<typename P>
	Relations parentsOf(FeatureBase<P> feature) const
	{
		return Relations(withParentRelationsOf(feature.ptr()));
	}
};

/// \endcond

} // namespace geodesk

