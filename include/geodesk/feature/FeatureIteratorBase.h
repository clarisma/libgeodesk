// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FeatureBase.h>
#include <geodesk/feature/MemberIterator.h>
#include <geodesk/feature/FeatureNodeIterator.h>
#include <geodesk/feature/WayCoordinateIterator.h>
#include <geodesk/filter/FeatureNodeFilter.h>
#include <geodesk/filter/WayNodeFilter.h>
#include <geodesk/query/Query.h>

#include "ParentRelationIterator.h"

namespace geodesk {

/// \cond

class GEODESK_API FeatureIteratorBase
{
public:
    explicit FeatureIteratorBase(const View& view);
    ~FeatureIteratorBase();

protected:
	const Feature& currentFeature() const { return current_; }
	void fetchNext();

private:
	void initNodeIterator(const View& view);
	void initParentWaysIterator(const View& view);
	void destroyParentWaysIterator();
	void initParentRelationsIterator(FeatureStore* store, FeaturePtr member,
		const MatcherHolder* matcher, const Filter* filter);
	void initParentRelationsIterator(const View& view);
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
			WayCoordinateIterator coords;
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

