// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "FeaturesBase.h"

namespace geodesk {

/// \cond
/// @brief A collection containing only Node features.
///
class Nodes : public FeaturesBase<Node>
{
public:
	using FeaturesBase::FeaturesBase;
	Nodes(const FeaturesBase<Feature>& other) : 
		FeaturesBase(withTypes(FeatureTypes::NODES)) {}
	Nodes(const FeaturesBase<Node>& other) : FeaturesBase(other) {}
	Nodes(const FeaturesBase<Way>& other) : FeaturesBase(getEmpty()) {}
	Nodes(const FeaturesBase<Relation>& other) : FeaturesBase(getEmpty()) {}

};

/// \endcond

} // namespace geodesk
