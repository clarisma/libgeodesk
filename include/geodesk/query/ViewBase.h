// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/query/Selection.h>
#include <geodesk/filter/BoundsFilter.h>
#include <geodesk/filter/ComboFilter.h>
#include <geodesk/feature/QueryException.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/geom/Box.h>

namespace geodesk {

class Filter;
class MatcherHolder;

/// \cond lowlevel

template <typename T>
class ViewBase : public Selection
{
public:
    enum
    {
        EMPTY,
        WORLD,
        WAY_NODES,
        MEMBERS,
        PARENTS
    };


    /*
    explicit ViewBase(FeatureStore* store) :
        Selection(EMPTY, 0, 0, store, {}, store->getAllMatcher(), nullptr)
    {
        store->addref();
        // TODO: this differs from other cons that steal the ref
    }
    */

    using Selection::Selection;

    ViewBase(const ViewBase& other) = default;

    void addref() const
    {
        store()->addref();
        matcher()->addref();
        if (filter()) filter()->addref();
    }

    void release() const
    {
        matcher()->release();
        if (filter()) filter()->release();
        store()->release();
            // Store must be released *last*, since the default matcher
            //  resides within it (cause of #20)
    }

    static auto related(int view, FeatureTypes types, const FeatureStore* store, FeaturePtr ptr, const char* query)
    {
        const MatcherHolder* matcher;
        int flags;
        if(query)
        {
            matcher = store->getMatcher(query);
            flags = USES_MATCHER;
            types &= matcher->acceptedTypes();
            if (types == 0) [[unlikely]]
            {
                view = EMPTY;
                // TODO: Do we need to release the matcher?
            }
        }
        else
        {
            matcher = store->getAllMatcher();
            flags = 0;
        }
        store->addref();
        return T::create(view, flags, types, store, ptr, matcher, nullptr);
    }

    static auto nodesOf(const FeatureStore* store, FeaturePtr ptr, const char* query = nullptr)
    {
        return related(WAY_NODES, FeatureTypes::NODES & FeatureTypes::WAYNODE_FLAGGED,
            store, ptr, query);
    }

    template<typename Related>
    auto withRelated(int kind, FeatureTypes acceptedTypes, Related related) const
    {
        if (kind_ != WORLD)
        {
            if (kind_ == EMPTY) return self().getEmpty();
            throw QueryException("Not implemented");
        }

        FeatureTypes types = types_ & acceptedTypes;
        if (types == 0) return self().getEmpty();

        const Filter* filter = filter_;
        if (flags_ & EXPLICIT_BOUNDS)
        {
            if (!tryGetBoundsFilter(&filter, bounds())) return self().getEmpty();
        }
        else
        {
            if (filter) filter->addref();
        }
        matcher_->addref();
        store_->addref();

        int flags = flags_ & ~(USES_BOUNDS | BOUNDS_ACTIVE);
        flags |= filter ? USES_FILTER : 0;

        return T::create(kind, flags, types, store_,
            related, matcher_, filter);
    }

    auto withNodesOf(WayPtr way) const
    {
        return withRelated(WAY_NODES,
            FeatureTypes::NODES & FeatureTypes::WAYNODE_FLAGGED, way);
    }

    // TODO: guard against empty relations
    static auto membersOf(const FeatureStore* store, FeaturePtr ptr, const char* query = nullptr)
    {
        return related(MEMBERS, FeatureTypes::RELATION_MEMBERS, store, ptr, query);
    }

    auto withMembersOf(RelationPtr rel) const
    {
        return withRelated(MEMBERS, FeatureTypes::RELATION_MEMBERS, rel);
    }

    auto withParentWaysOf(Coordinate anonNode) const
    {
        return withRelated(PARENTS, FeatureTypes::WAYS, anonNode);
    }

    auto withParentsOf(FeaturePtr feature) const
    {
        FeatureTypes parentTypes = 0;
        if (feature.isNode())
        {
            // nodes can have both ways and relations as parents
            if (feature.flags() & FeatureFlags::WAYNODE)
            {
                parentTypes = FeatureTypes::WAYS & FeatureTypes::WAYNODE_FLAGGED;
            }
        }
        if (feature.isRelationMember())
        {
            parentTypes |= FeatureTypes::RELATIONS;
        }
        return withRelated(PARENTS, parentTypes, feature);
    }

    auto withParentRelationsOf(FeaturePtr feature) const
    {
        if (!feature.isRelationMember()) return self().getEmpty();
        return withRelated(PARENTS, FeatureTypes::RELATIONS, feature);
    }

    static auto parentWaysOf(const FeatureStore* store, Coordinate anonNode, const char* query = nullptr)
    {
        const MatcherHolder* matcher;
        int flags;
        if(query)
        {
            matcher = store->getMatcher(query);
            flags = USES_MATCHER;
        }
        else
        {
            matcher = store->getAllMatcher();
            flags = 0;
        }
        store->addref();
        // important! cannot be waynode flagged, because that would
        // only return ways that have at least one feature node
        // For parents of anonymous nodes, we must query *all* ways
        return T::create(PARENTS, flags, FeatureTypes::WAYS,
            store, anonNode, matcher, nullptr);
    }

    static auto parentsOf(const FeatureStore* store, FeaturePtr feature, FeatureTypes types, const char* query = nullptr)
    {
        assert((types & ((FeatureTypes::WAYS & FeatureTypes::WAYNODE_FLAGGED) |
            FeatureTypes::RELATIONS)) == types);
            // accepted types must not be anything other than
            // ways (only with feature nodes) and relations
        return related(PARENTS, types, store, feature, query);
    }

    /*
    static auto parentsOf(FeatureStore* store, FeaturePtr feature, const char* query = nullptr)
    {
        FeatureTypes types = feature.isRelationMember() ? FeatureTypes::RELATIONS : 0;
        types |= feature.isType(FeatureTypes::NODES & FeatureTypes::WAYNODE_FLAGGED) ?
            FeatureTypes::WAYS : 0;
        if (types == 0) return self().getEmpty();
    }
    */

    auto withTypes(FeatureTypes types) const
    {
        types &= types_;
        if (types == 0) return self().getEmpty();
        store_->addref();
        matcher_->addref();
        if (filter_) filter_->addref();
        return T::create(kind_, flags_, types, store_, context_, matcher_, filter_);
    }

    auto withQuery(const char* query, FeatureTypes newTypes = FeatureTypes::ALL) const
    {
        // TODO: Turn ParseException into QueryException
        //try
        //{
            const MatcherHolder* newMatcher = store_->getMatcher(query);
            newTypes &= types_ & newMatcher->acceptedTypes();
            if (newTypes == 0)
            {
                newMatcher->release();
                return self().getEmpty();
            }
            if (flags_ & USES_MATCHER)
            {
                matcher_->addref();
                newMatcher = MatcherHolder::combine(matcher_, newMatcher);
                // combine() steals references
            }

            if (filter_) filter_->addref();     
            store_->addref();
            return T::create(kind_, flags_ | USES_MATCHER, newTypes, store_,
                context_, newMatcher, filter_);
        // }
        /*
        catch (const ParseException& ex)
        {
            // Environment::get().raiseQueryException("Bad query: %s", ex.what());
            Environment::get().raiseQueryException(ex.what());
            return NULL;
        }
        */
    }

    /// Returns a new View constricted by the given filter
    ///
    /// @param newFilter new reference (will be consumed), non-null
    ///
    auto withFilter(const Filter* newFilter) const
    {
        FeatureTypes newTypes = types_ & newFilter->acceptedTypes();
        if (newTypes == 0)
        {
            newFilter->release();
            return self().getEmpty();
        }
        const Filter* f;
        if (!ComboFilter::tryCombine(&f, filter_, newFilter))
        {
            newFilter->release();
            return self().getEmpty();
        }
        newFilter->release();

        matcher_->addref();      // createWith consumes ref to matcher
        store_->addref();

        if(flags_ & USES_BOUNDS)
        {
            Box b = bounds();
            int boundsActiveFlag = flags_ & BOUNDS_ACTIVE;
            if (boundsActiveFlag)
            {
                int filterFlags = f->flags();
                if (filterFlags & FilterFlags::USES_BBOX)
                {
                    const SpatialFilter* spatialFilter =
                        SpatialFilter::cast(f);
                    if (filterFlags & FilterFlags::STRICT_BBOX)
                    {
                        b = Box::simpleIntersection(b, spatialFilter->bounds());
                    }
                    else
                    {
                        b = Box::simpleSmaller(b, spatialFilter->bounds());
                    }
                }
            }
            else
            {
                if (f->flags() & FilterFlags::USES_BBOX)
                {
                    b = SpatialFilter::cast(f)->bounds();
                    boundsActiveFlag = b != Box::ofWorld() ? BOUNDS_ACTIVE : 0;
                }
            }
            Context ctx;
            ctx.bounds = { b.minX(), b.minY(), b.maxX(), b.maxY() };
            return T::create(kind_, flags_ | USES_FILTER | boundsActiveFlag,
                newTypes, store_, ctx, matcher_, f);
        }
        return T::create(kind_, flags_ | USES_FILTER, newTypes, store_,
            context_, matcher_, f);
    }

    auto withBounds(Box box) const
    {
        if (box.isEmpty()) [[unlikely]]
        {
            return self().getEmpty();
        }
        if (flags_ & USES_BOUNDS)  [[likely]]
        {
            if (flags_ & BOUNDS_ACTIVE)
            {
                box = Box::simpleIntersection(box, bounds());
                if(box.isEmpty()) return self().getEmpty();
            }
            matcher_->addref();
            if(filter_) filter_->addref();
            store_->addref();
            return T::create(kind_, flags_ | BOUNDS_ACTIVE | EXPLICIT_BOUNDS,
                types_, store_, box, matcher_, filter_);
        }
        throw QueryException("Not yet implemented");
    }

    auto withBounds(Coordinate xy) const
    {
        return withBounds(Box(xy));
    }

    // TODO:
    // Combination rules:
    // - if one of the Views is EMPTY, return EMPTY
    // - if one of the Views is WORLD, graft the matcher/filter/types
    //   onto the other View (which is a Related View)
    //   - if type intersection is 0, return EMPTY
    //   - if the WORLD View has an active bbox, that bbox needs
    //     to be turned into a filter for the Related View
    //     (since Related Views don't have a native bbox)
    // - Otherwise, *both* View are Related Views:
    //   - if type intersection is 0, return EMPTY
    //   - else materialize one of the Views and use it as a
    //     typedId set Matcher for the other

    auto withOther(const ViewBase& other) const
    {
        // Cannot combine sets from different stores
        if (store_ != other.store_) return self().getEmpty();

        FeatureTypes newTypes = types_ & other.types_;
        if (newTypes == 0) return self().getEmpty();

        if (kind_ != WORLD || other.kind_ != WORLD)
        {
            // Others will require special handling, rarely used
            // TODO: allow other combinations
            throw QueryException("Not yet implented");
        }

        const Filter* newFilter;
        if (!ComboFilter::tryCombine(&newFilter, filter_, other.filter_))
        {
            return self().getEmpty();
        }

        Box b = bounds();
        if ((flags_ | other.flags_) & BOUNDS_ACTIVE)
        {
            b = Box::simpleIntersection(b, other.bounds());
        }

        const MatcherHolder* newMatcher;
        if (flags_ & USES_MATCHER)
        {
            if (other.flags_ & USES_MATCHER)
            {
                matcher_->addref();
                other.matcher_->addref();
                newMatcher = MatcherHolder::combine(
                    matcher_, other.matcher_);
                // combine() steals the references
            }
            else
            {
                newMatcher = matcher_;
                newMatcher->addref();
            }
        }
        else
        {
            newMatcher = other.matcher_;
            newMatcher->addref();
        }

        store_->addref();
        return T::create(kind_, flags_ | other.flags_, newTypes, store_,
            b, newMatcher, newFilter);
    }

    const T& self() const noexcept
    {
        return static_cast<const T&>(*this);
    }

private:
    bool tryGetBoundsFilter(const Filter** res, Box b) const
    {
        const Filter* filter = filter_;
        const Filter* boundsFilter = new BoundsFilter(b);
        bool success = ComboFilter::tryCombine(res, boundsFilter, filter);
        boundsFilter->release();
        return success;
    }
};

/// \endcond lowlevel

} // namespace geodesk
