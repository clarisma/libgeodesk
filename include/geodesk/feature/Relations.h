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
		FeaturesBase(withTypes(FeatureTypes::RELATIONS)) {}
	Relations(const FeaturesBase<Node>& other) : FeaturesBase(getEmpty()) {}
	Relations(const FeaturesBase<Way>& other) : FeaturesBase(getEmpty()) {}
	Relations(const FeaturesBase<Relation>& other) : FeaturesBase(other) {}

	template<typename P>
	Relations parentsOf(FeatureBase<P> feature) const
	{
		// if(!feature.belongsToRelation()) return Relations(empty());
		return Relations(parentsOf(feature.ptr()));
		// TODO: may be able to optimize, this parent view can only
		//  contain relations
	}

	template<typename P>
	Relations membersOf(FeatureBase<P> feature) const
	{
		// Relations can only be members of other relations
		if(!feature.isRelation()) return Relations(getEmpty());
		return Relations(membersOf(RelationPtr(feature.ptr())));
	}

};

/// \endcond

} // namespace geodesk
