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
		FeaturesBase(View(other.view_ & FeatureTypes::NODES)) {}
	Nodes(const FeaturesBase<Node>& other) : FeaturesBase(other.view_) {}
	Nodes(const FeaturesBase<Way>& other) : FeaturesBase(empty()) {}
	Nodes(const FeaturesBase<Relation>& other) : FeaturesBase(empty()) {}

};

/// \endcond

} // namespace geodesk
