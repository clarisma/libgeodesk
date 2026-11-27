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
		FeaturesBase(other.withTypes(FeatureTypes::NODES)) {}
	Nodes(const FeaturesBase<Node>& other) : FeaturesBase(other) {}
	Nodes(const FeaturesBase<Way>& other) : FeaturesBase(other.getEmpty()) {}
	Nodes(const FeaturesBase<Relation>& other) : FeaturesBase(other.getEmpty()) {}

};

/// \endcond

} // namespace geodesk
