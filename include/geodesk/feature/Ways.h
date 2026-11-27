// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "FeaturesBase.h"

namespace geodesk {

/// \cond
/// @brief A collection containing only Way features.
///
class Ways : public FeaturesBase<Way>
{
public:
	using FeaturesBase::FeaturesBase;
	Ways(const FeaturesBase<Feature>& other) :
		FeaturesBase(withTypes(FeatureTypes::WAYS)) {}
	Ways(const FeaturesBase<Node>& other) : FeaturesBase(getEmpty()) {}
	Ways(const FeaturesBase<Way>& other) : FeaturesBase(other) {}
	Ways(const FeaturesBase<Relation>& other) : FeaturesBase(getEmpty()) {}
};

/// \endcond

} // namespace geodesk

