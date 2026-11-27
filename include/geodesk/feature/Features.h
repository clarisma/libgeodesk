// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#ifndef GEODESK_DOXYGEN

#include "FeaturesBase.h"
#include "Nodes.h"
#include "Ways.h"
#include "Relations.h"

namespace geodesk {

class Features : public FeaturesBase<Feature>
{
public:
	using FeaturesBase::FeaturesBase;
	Features(const char* golFile) :
		FeaturesBase(rootView(golFile))
	{
	}

	template <typename T>
	Features(const FeaturesBase<T>& other) :
		FeaturesBase(other)
	{
	}

	///
	/// Returns a view of this collection that contains only nodes.
	///
	Nodes nodes() const { return Nodes(withTypes(FeatureTypes::NODES)); };
	
	///
	/// Returns a view of this collection that contains only nodes 
	/// matching the given query.
	/// 
	/// @param query a query in <a href="https://docs.geodesk.com/goql">GOQL</a> format 
	/// 
	Nodes nodes(const char* query) const
	{
		return Nodes(withQuery(query, FeatureTypes::NODES));
	}
	Ways ways() const {	return Ways(withTypes(FeatureTypes::WAYS)); }
	Ways ways(const char* query) const
	{
		return Ways(withQuery(query, FeatureTypes::WAYS));
	}
	Relations relations() const { return Relations(withTypes(FeatureTypes::RELATIONS)); }
	Relations relations(const char* query) const
	{
		return Relations(withQuery(query, FeatureTypes::RELATIONS));
	}

};

} // namespace geodesk

#endif