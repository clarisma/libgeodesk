// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include "OpGraph.h"
#include "TagClause.h"

namespace geodesk {

struct TagClause;

struct Selector
{
public:
	Selector(FeatureTypes types);

	void addClause(TagClause* clause);

	Selector* next;
	FeatureTypes acceptedTypes;
	uint32_t indexBits;
	TagClause* firstClause;
	OpNode falseOp;
};

} // namespace geodesk
