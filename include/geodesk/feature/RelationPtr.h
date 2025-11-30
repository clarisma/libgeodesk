// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <unordered_set>
#include <geodesk/feature/FeaturePtr.h>

namespace geodesk {

/// \cond lowlevel
///
class RelationPtr : public FeaturePtr
{
public:
	RelationPtr() {}
	explicit RelationPtr(FeaturePtr f) : FeaturePtr(f.ptr()) { assert(isNull() || isRelation()); }
	explicit RelationPtr(DataPtr p) : FeaturePtr(p) {}

	bool containsPointFast(double cx, double cy);
};

// TODO: remove in v2
/**
 * A safeguard against endless recursion in case a Relation contains a
 * reference cycle.
 */
class RecursionGuard
{
public:
	/**
	 * Creates a recursion guard for the given parent relation.
	 */
	RecursionGuard(RelationPtr relation) : parent_(relation.idBits()) {}

	/**
	 * Checks if we've already seen the given relation, and adds it to the
	 * set if we're processing it for the first time.
	 *
	 * @return true if it's ok to process the child relation, false if
	 *     we've already processed it (i.e. it refers back to a parent)
	 */
	bool checkAndAdd(RelationPtr child)
	{
		uint64_t childIdBits = child.idBits();
		if (childIdBits == parent_) return false;
		if (childRelations_.find(childIdBits) != childRelations_.end())
		{
			return false;
		}
		childRelations_.insert(childIdBits);
		return true;
	}

private:
	uint64_t parent_;
	std::unordered_set<int64_t> childRelations_;
};

/// \endcond
} // namespace geodesk
