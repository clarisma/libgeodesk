// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/FeatureIteratorBase.h>
#include <geodesk/feature/View.h>

namespace geodesk {

enum IterType
{
    EMPTY,
    WORLD,
    WAYNODES_ALL,
    WAYNODES_FEATURES,
    RELATION_MEMBERS,
    PARENTS_ALL,
    PARENTS_WAYS,
    PARENTS_RELATIONS
};


FeatureIteratorBase::FeatureIteratorBase(const View& view) :
    current_(view.store())
{
    switch (view.view())
    {
    case View::EMPTY:
        type_ = EMPTY;
        break;
    case View::WAY_NODES:
        initNodeIterator(view);
        break;
    case View::WORLD:
        type_ = WORLD;
        new (&storage_.worldQuery) Query(view.store(), view.bounds(),
            view.types(), view.matcher(), view.filter());
        break;
    case View::MEMBERS:
        type_ = RELATION_MEMBERS;
        new (&storage_.members) MemberIterator(view.store(),
            RelationPtr(view.relatedFeature()).bodyptr(),
            view.types(), view.matcher(), view.filter());
        break;
    case View::PARENTS:
        if (view.types() & FeatureTypes::WAYS)
        {
            initParentWaysIterator(view);   // sets type_
            type_ = (view.types() & FeatureTypes::RELATIONS)?
                PARENTS_ALL : PARENTS_WAYS;
        }
        else
        {
            assert(view.types() & FeatureTypes::RELATIONS);
            initParentRelationsIterator(view);  // sets type_
        }
        break;
    }
    fetchNext();
}

void FeatureIteratorBase::initNodeIterator(const View& view)
{
    WayPtr way(view.relatedFeature());
    if(way.hasFeatureNodes())
    {
        type_ = view.usesMatcher() ? WAYNODES_FEATURES : WAYNODES_ALL;
        new(&storage_.nodes.featureNodes) FeatureNodeIterator(view.store());
        storage_.nodes.featureNodes.start(way.bodyptr(), way.flags(),
            view.matcher(), view.filter());
        storage_.nodes.nextFeatureNode = storage_.nodes.featureNodes.next();
        current_.setType(Feature::ExtendedFeatureType::NODE);
    }
    else
    {
        type_ = view.usesMatcher() ? EMPTY : WAYNODES_ALL;
        storage_.nodes.nextFeatureNode = NodePtr();
        // remember, we need to explicitly initialize fields within
        // a union, since no default initialization takes place
    }
    if(type_ == WAYNODES_ALL)
    {
        new(&storage_.nodes.coords) WayCoordinateIterator(way);
    }
}

void FeatureIteratorBase::initParentWaysIterator(const View& view)
{
    type_ = PARENTS_WAYS;
    NodePtr node(view.relatedFeature());
    Coordinate xy;
    Filter* filter;
    if (!node.isNull())
    {
        xy = node.xy();
        new(&storage_.parents.featureNodeFilter) FeatureNodeFilter(node, view.filter());
        filter = &storage_.parents.featureNodeFilter;
    }
    else
    {
        xy = view.relatedAnonymousNode();
        new(&storage_.parents.wayNodeFilter) WayNodeFilter(xy, view.filter());
        filter = &storage_.parents.wayNodeFilter;
    }
    new (&storage_.parents.parentWayQuery) Query(
        view.store(), Box(xy),
        view.types() & FeatureTypes::WAYS, view.matcher(), filter);
}

void FeatureIteratorBase::initParentRelationsIterator(FeatureStore* store, FeaturePtr member,
    const MatcherHolder* matcher, const Filter* filter)
{
    type_ = PARENTS_RELATIONS;
    new(&storage_.parents.parentRelations) ParentRelationIterator(
        store, member.relationTableFast(), matcher, filter);
}


void FeatureIteratorBase::initParentRelationsIterator(const View& view)
{
    initParentRelationsIterator(view.store(), view.relatedFeature(),
        view.matcher(), view.filter());
}

void FeatureIteratorBase::destroyParentWaysIterator()
{
    storage_.parents.parentWayQuery.~Query();
       // no need to destroy FeatureNodeFilter/WayNodeFilter,
        // since they do not require any cleanup
}

FeatureIteratorBase::~FeatureIteratorBase()
{
    switch (type_)
    {
    case EMPTY:
        break;
    case WORLD:
        storage_.worldQuery.~Query();
        break;
    case WAYNODES_ALL:
        storage_.nodes.coords.~WayCoordinateIterator();
        // fall through
    case WAYNODES_FEATURES:
        storage_.nodes.featureNodes.~FeatureNodeIterator();
        break;
    case RELATION_MEMBERS:
        storage_.members.~MemberIterator();
        break;
    case PARENTS_ALL:   // fallthrough
    case PARENTS_WAYS:
        destroyParentWaysIterator();
            // A PARENTS_ALL query starts with ways, then switches to
            // PARENT_RELATIONS once all ways have been iterated
        break;
    case PARENTS_RELATIONS:
        storage_.parents.parentRelations.~ParentRelationIterator();
        break;
    }
}

bool FeatureIteratorBase::fetchNextParentWay()
{
    FeaturePtr next = storage_.parents.parentWayQuery.next();
    if(next.isNull())
    {
        current_.setNull();
        return false;
    }
    assert(next.isWay());
    current_.setTypedFeature(next);
    return true;
}

void FeatureIteratorBase::fetchNextParentRelation()
{
    RelationPtr next = storage_.parents.parentRelations.next();
    if(next.isNull())
    {
        current_.setNull();
    }
    else
    {
        current_.setTypedFeature(next);
    }
}

void FeatureIteratorBase::switchToParentRelationsIterator()
{
    assert(type_ == PARENTS_ALL);
    FeatureStore* store = storage_.parents.parentWayQuery.store();
    NodePtr node = storage_.parents.featureNodeFilter.node();
    const MatcherHolder* matcher = storage_.parents.parentWayQuery.matcher();
    const Filter* filter = storage_.parents.featureNodeFilter.secondaryFilter();
    destroyParentWaysIterator();
    initParentRelationsIterator(store, node, matcher, filter);
}

void FeatureIteratorBase::fetchNext()
{
    switch (type_)
    {
    case EMPTY:
        current_.setNull();
        return;
    case WORLD:
    {
        FeaturePtr next = storage_.worldQuery.next();
        if(next.isNull())
        {
            current_.setNull();
        }
        else
        {
            current_.setTypedFeature(next);
        }
        return;
    }
    case WAYNODES_FEATURES:
    {
        NodePtr next = storage_.nodes.nextFeatureNode;
        if(next.isNull())
        {
            current_.setNull();
        }
        else
        {
            current_.setFeature(next);
            storage_.nodes.nextFeatureNode = storage_.nodes.featureNodes.next();
        }
        return;
    }
    case WAYNODES_ALL:
    {
        Coordinate xy = storage_.nodes.coords.next();
        if(xy.isNull())
        {
            current_.setNull();
        }
        else
        {
            NodePtr next = storage_.nodes.nextFeatureNode;
            if(!next.isNull())
            {
                if(next.xy() == xy)
                {
                    storage_.nodes.nextFeatureNode =
                        storage_.nodes.featureNodes.next();
                    current_.setType(Feature::ExtendedFeatureType::NODE);
                    current_.setFeature(next);
                    return;
                }
            }
            current_.setType(Feature::ExtendedFeatureType::ANONYMOUS_NODE);
            current_.setIdAndXY(0, xy);
        }
        return;
    }
    case RELATION_MEMBERS:
    {
        FeaturePtr next = storage_.members.next();
        if(next.isNull())
        {
            current_.setNull();
        }
        else
        {
            current_.setTypedFeature(next);
            current_.setRole(storage_.members.currentRoleStr());
        }
        return;
    }
    case PARENTS_ALL:
    {
        if (!fetchNextParentWay())
        {
            switchToParentRelationsIterator();
            assert(type_ == PARENTS_RELATIONS);
            fetchNextParentRelation();
        }
        return;
    }
    case PARENTS_WAYS:
    {
        fetchNextParentWay();
        return;
    }
    case PARENTS_RELATIONS:
    {
        fetchNextParentRelation();
        return;
    }
    }
}

} // namespace geodesk