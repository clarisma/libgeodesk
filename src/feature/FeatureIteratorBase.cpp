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
    RELATION_MEMBERS
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
    }
    fetchNext();
}

void FeatureIteratorBase::initNodeIterator(const View& view)
{
    WayPtr way(view.relatedFeature());
    if(way.hasFeatureNodes())
    {
        type_ = view.usesMatcher() ? WAYNODES_FEATURES : WAYNODES_ALL;
        new(&storage_.nodes.featureNodes) FeatureNodeIterator(
            view.store(), way.bodyptr(), way.flags(),
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
    }
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
    }
}

} // namespace geodesk