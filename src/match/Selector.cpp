// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include "Selector.h"

namespace geodesk {

Selector::Selector(FeatureTypes types) :
	next(nullptr),
	acceptedTypes(types),
	indexBits(0),
	firstClause(nullptr),
	falseOp(Opcode::RETURN)
{
}


void Selector::addClause(TagClause* clause)
{
	TagClause** pNext = &firstClause;
	TagClause* current;
	while ((current = *pNext) != nullptr)
	{
		int comp = current->keyOp.compareTo(&clause->keyOp);
		if (comp < 0)
		{
			pNext = &current->next;
			continue;
		}
		if (comp == 0)
		{
			current->absorb(clause);
			// The absorbed clause may require the key even if the
			// existing one doesn't (e.g. "[k!=a][k]"), so we need
			// to check for index bits here as well (#55)
			if (current->flags & TagClause::KEY_REQUIRED)
			{
				indexBits |= IndexBits::fromCategory(current->category);
			}
			return;
		}
		break;
	}
	if (clause->flags & TagClause::KEY_REQUIRED)
	{
		indexBits |= IndexBits::fromCategory(clause->category);
	}
	clause->next = current;
	*pNext = clause;
}


} // namespace geodesk
