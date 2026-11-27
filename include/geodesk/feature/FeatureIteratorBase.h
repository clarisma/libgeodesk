// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FeatureBase.h>
#include <geodesk/feature/MemberIterator.h>
#include <geodesk/feature/FeatureNodeIterator.h>
#include <geodesk/feature/WayNodeCursor.h>
#include <geodesk/filter/FeatureNodeFilter.h>
#include <geodesk/filter/WayNodeFilter.h>
#include <geodesk/query/Query.h>

#include "ParentRelationIterator.h"

#ifdef GEODESK_PYTHON
#include <Python.h>
#endif


namespace geodesk {

/// \cond

class GEODESK_API FeatureIteratorBase
{
public:
    explicit FeatureIteratorBase(const Selection& view);
    ~FeatureIteratorBase();

	const Feature& current() const { return current_; }	// TODO
#ifdef GEODESK_PYTHON
	PyObject* currentRole() const;
#endif

protected:
	const Feature& currentFeature() const { return current_; }
	void fetchNext();

private:
	void initNodeIterator(const Selection& view);
	void initParentWaysIterator(const Selection& view);
	void destroyParentWaysIterator();
	void initParentRelationsIterator(FeatureStore* store, FeaturePtr member,
		const MatcherHolder* matcher, const Filter* filter);
	void initParentRelationsIterator(const Selection& view);
	void switchToParentRelationsIterator();
	bool fetchNextParentWay();
	void fetchNextParentRelation();

	uint_fast8_t type_;
    Feature current_;
	union Storage
	{
		Query worldQuery;
		MemberIterator members;
		struct
		{
			WayNodeCursor cursor;
			FeatureNodeIterator featureNodes;
			NodePtr nextFeatureNode;
		} nodes;
		struct
		{
			union
			{
				Query parentWayQuery;
				ParentRelationIterator parentRelations;
			};
			union
			{
				FeatureNodeFilter featureNodeFilter;
				WayNodeFilter wayNodeFilter;
			};
		} parents;

	    // Default constructor
	    Storage() {}
	    // Destructor does nothing as we handle destruction manually
	    ~Storage() {}
	}
	storage_;
};


// \endcond

} // namespace geodesk


// Possible iterators:
// - empty -> nothing
// - world view -> Node,Way,Relation
// - purely coordinate nodes -> AnonymousNode
// - purely feature nodes -> Node
// - mix of feature nodes and coordinate nodes -> Node, AnonymousNode
// - members -> Node,Way,Relation
// - parent relations -> Relation
// - parent ways (type of world view) -> Way
// - both parent relations & parent ways -> Way,Relation

