// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "FeaturesBase.h"

namespace geodesk {

/// \cond
/// @brief A collection containing only Relation features.
///
class Relations : public FeaturesBase<Relation>
{
public:
	using FeaturesBase::FeaturesBase;
	Relations(const FeaturesBase<Feature>& other) :
		FeaturesBase(View(other.view_& FeatureTypes::RELATIONS)) {}
	Relations(const FeaturesBase<Node>& other) : FeaturesBase(empty()) {}
	Relations(const FeaturesBase<Way>& other) : FeaturesBase(empty()) {}
	Relations(const FeaturesBase<Relation>& other) : FeaturesBase(other.view_) {}

	template<typename P>
	Relations parentsOf(FeatureBase<P> feature) const
	{
		// if(!feature.belongsToRelation()) return Relations(empty());
		return Relations(view_.parentsOf(feature.ptr()));
		// TODO: may be able to optimize, this parent view can only
		//  contain relations
	}

	template<typename P>
	Relations membersOf(FeatureBase<P> feature) const
	{
		// Relations can only be members of other relations
		if(!feature.isRelation()) return Relations(empty());
		return Relations(view_.membersOfRelation(feature.ptr()));
	}

};

/// \endcond

} // namespace geodesk
