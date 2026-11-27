// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/ComboFilter.h>
#include <geodesk/feature/QueryException.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/geom/Box.h>

#include "geodesk/filter/BoundsFilter.h"

namespace geodesk {

class Filter;
class MatcherHolder;

/// \cond lowlevel

class GEODESK_API View
{
    union Context
    {
        struct Related
        {
            FeaturePtr feature;
            Coordinate xy;
        };

        Context() noexcept {}
        Context(Box b) : bounds(b) {}
        Context(FeaturePtr f) : related{f,Coordinate()} {}
        Context(Coordinate xy) : related{FeaturePtr(),xy} {}

        Box bounds;         // If used, USES_BOUNDS flag must be set
        Related related;    // If used, USES_BOUNDS flag must be clear
    };

    enum Flags
    {
        /**
         * Context uses `bounds` (instead of `relatedFeature` in
         * the same union). Only used by WORLD selection.
         */
        USES_BOUNDS = 1,
        /**
         * `bounds` has bounds tighter than `Box::ofWorld`. `USES_BOUNDS` must be set.
         */
        BOUNDS_ACTIVE = 2,
        EXPLICIT_BOUNDS = 4,
        USES_MATCHER = 8,
        USES_FILTER = 16,

        // TODO: need flag to indicate if relatedFeature is in use
        // or does NOT USES_BOUNDS imply use of relatedFeature?
    };

public:
    enum
    {
        EMPTY,
        WORLD,
        WAY_NODES,
        MEMBERS,
        PARENTS
    };

    explicit View(FeatureStore* store) :
        view_(EMPTY), flags_(0), types_(0), store_(store),
        matcher_(store->getAllMatcher()), filter_(nullptr)
    {
        store->addref();
        // TODO: this differs from other cons that steal the ref
    }


    // steals references
    View(int view, int flags, FeatureTypes types, FeatureStore* store,
        Context context, const MatcherHolder* matcher, const Filter* filter) :
        view_(view), flags_(flags), types_(types), store_(store), 
        context_(context), matcher_(matcher), filter_(filter)
    {
        //printf("Creating view, store refcount = %llu\n", store_->refcount());
        //fflush(stdout);
    }

    View(const View& other) :
        View(other.view_, other.flags_, other.types_, other.store_,
            other.context_, other.matcher_, other.filter_)
    {
        // printf("Creating copy of view, store refcount before = %llu\n", store_->refcount());
        // fflush(stdout);
        // caller must inc refcounts, but here, this method is
        // the caller
        store_->addref();
        matcher_->addref();
        if (filter_) filter_->addref();
        //printf("  store refcount after = %llu\n", store_->refcount());
        //fflush(stdout);
    }

    ~View()
    {
        //printf("Destroying view, store refcount before = %llu\n", store_->refcount());
        //fflush(stdout);

        matcher_->release();
        if (filter_) filter_->release();
        store_->release();
            // Store must be released *last*, since the default matcher
            //  resides within it (cause of #20)
    }

    View& operator=(const View& other)
    {
        //printf("Assigning view (store refcount before = %llu)\n", store_->refcount());
        //fflush(stdout);
        view_ = other.view_;
        flags_ = other.flags_;
        types_ = other.types_;
        context_ = other.context_;
        if(matcher_ != other.matcher_)
        {
            other.matcher_->addref();
            matcher_->release();
            matcher_ = other.matcher_;
        }
        if(filter_ != other.filter_)
        {
            if(other.filter_) other.filter_->addref();
            if(filter_) filter_->release();
            filter_ = other.filter_;
        }
        if(store_ != other.store_)
        {
            other.store_->addref();
            store_->release();
            store_ = other.store_;
        }
        // printf("  Assigned. (store refcount now = %llu)\n", store_->refcount());
        // fflush(stdout);
        return *this; // Return the current object
    }

    static View related(int view, FeatureTypes types, FeatureStore* store, FeaturePtr ptr, const char* query)
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
        return { view, flags, types, store, ptr, matcher, nullptr };
    }

    static View nodesOf(FeatureStore* store, FeaturePtr ptr, const char* query = nullptr)
    {
        return related(WAY_NODES, FeatureTypes::NODES & FeatureTypes::WAYNODE_FLAGGED,
            store, ptr, query);
    }

    template<typename Related>
    View withRelated(int kind, FeatureTypes acceptedTypes, Related related) const
    {
        if (view_ != WORLD)
        {
            if (view_ == EMPTY) return empty();
            throw QueryException("Not implemented");
        }

        FeatureTypes types = types_ & acceptedTypes;
        if (types == 0) return empty();

        const Filter* filter = filter_;
        if (flags_ & EXPLICIT_BOUNDS)
        {
            if (!tryGetBoundsFilter(&filter, bounds())) return empty();
        }
        else
        {
            if (filter) filter->addref();
        }
        matcher_->addref();
        store_->addref();

        int flags = flags_ & ~(USES_BOUNDS | BOUNDS_ACTIVE);
        flags |= filter ? USES_FILTER : 0;

        return { kind, flags, types, store_,
            related, matcher_, filter };
    }

    View nodesOf(WayPtr way) const
    {
        return withRelated(WAY_NODES,
            FeatureTypes::NODES & FeatureTypes::WAYNODE_FLAGGED, way);
    }

    // TODO: guard against empty relations
    static View membersOf(FeatureStore* store, FeaturePtr ptr, const char* query = nullptr)
    {
        return related(MEMBERS, FeatureTypes::RELATION_MEMBERS, store, ptr, query);
    }

    View membersOf(RelationPtr rel) const
    {
        return withRelated(MEMBERS, FeatureTypes::ALL, rel);
    }

    View parentWaysOf(Coordinate anonNode) const
    {
        return withRelated(PARENTS, FeatureTypes::WAYS, anonNode);
    }

    View parentsOf(FeaturePtr feature) const
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

    static View parentWaysOf(FeatureStore* store, Coordinate anonNode, const char* query = nullptr)
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
        return { PARENTS, flags, FeatureTypes::WAYS,
            store, anonNode, matcher, nullptr };
    }

    static View parentsOf(FeatureStore* store, FeaturePtr feature, int types, const char* query = nullptr)
    {
        assert((types & ((FeatureTypes::WAYS & FeatureTypes::WAYNODE_FLAGGED) |
            FeatureTypes::RELATIONS)) == types);
            // accepted types must not be anything other than
            // ways (only with feature nodes) and relations
        return related(PARENTS, types, store, feature, query);
    }

    uint32_t view() const noexcept { return view_; }
    FeatureStore* store() const noexcept { return store_; }
    FeatureTypes types() const noexcept { return types_; }
    const MatcherHolder* matcher() const noexcept { return matcher_; }
    const Filter* filter() const { return filter_; }
    Box bounds() const noexcept
    { 
        return context_.bounds;
    }

    FeaturePtr relatedFeature() const noexcept
    {
        return context_.related.feature;
    }

    Coordinate relatedAnonymousNode() const noexcept
    {
        return context_.related.xy;
    }

    bool usesMatcher() const noexcept
    {
        return flags_ & USES_MATCHER;
    }

    bool usesFilter() const noexcept
    {
        return flags_ & USES_FILTER;
    }

    bool usesMatcherOrFilter() const noexcept
    {
        return flags_ & (USES_MATCHER | USES_FILTER);
    }

    /* // TODO: bad?
    View& operator=(const View& other)
    {
        view_ = other.view_;
        flags_ = other.flags_;
        types_ = other.types_;
        other.store_->addref();
        other.matcher_->addref();
        if (other.filter_) other.filter_->addref();
        store_->release();
        matcher_->release();
        if (filter_) filter_->release();
        store_ = other.store_;
        context_ = other.context_;
        matcher_ = other.matcher_;
        filter_ = other.filter_;
        return *this;
    }
    */

    View operator&(FeatureTypes types) const
    {
        types &= types_;
        if (types == 0) return empty();
        store_->addref();
        matcher_->addref();
        if (filter_) filter_->addref();
        return View(view_, flags_, types, store_, context_, matcher_, filter_);
    }

    View withQuery(const char* query, FeatureTypes newTypes = FeatureTypes::ALL) const
    {
        // TODO: Turn ParseException into QueryException
        //try
        //{
            const MatcherHolder* newMatcher = store_->getMatcher(query);
            newTypes &= types_ & newMatcher->acceptedTypes();
            if (newTypes == 0)
            {
                newMatcher->release();
                return empty();
            }
            if (flags_ & USES_MATCHER)
            {
                matcher_->addref();
                newMatcher = MatcherHolder::combine(matcher_, newMatcher);
                // combine() steals references
            }

            if (filter_) filter_->addref();     
            store_->addref();
            return View(view_, flags_ | USES_MATCHER, newTypes, store_,
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
    View withFilter(const Filter* newFilter) const
    {
        FeatureTypes newTypes = types_ & newFilter->acceptedTypes();
        if (newTypes == 0)
        {
            newFilter->release();
            return empty();
        }
        const Filter* f;
        if (!ComboFilter::tryCombine(&f, filter_, newFilter))
        {
            newFilter->release();
            return empty();
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
            return View(view_, flags_ | USES_FILTER | boundsActiveFlag,
                newTypes, store_, ctx, matcher_, f);
        }
        return View(view_, flags_ | USES_FILTER, newTypes, store_,
            context_, matcher_, f);
    }

    View withBounds(Box box) const
    {
        if (box.isEmpty()) [[unlikely]]
        {
            return empty();
        }
        if (flags_ & USES_BOUNDS)  [[likely]]
        {
            if (flags_ & BOUNDS_ACTIVE)
            {
                box = Box::simpleIntersection(box, bounds());
                if(box.isEmpty()) return empty();
            }
            matcher_->addref();
            if(filter_) filter_->addref();
            store_->addref();
            return View(view_, flags_ | BOUNDS_ACTIVE | EXPLICIT_BOUNDS,
                types_, store_, box, matcher_, filter_);
        }
        throw QueryException("Not yet implemented");
    }

    View withBounds(Coordinate xy) const
    {
        if (flags_ & USES_BOUNDS)
        {
            if (flags_ & BOUNDS_ACTIVE)
            {
                if(!bounds().contains(xy)) return empty();
            }
            matcher_->addref();
            if(filter_) filter_->addref();
            store_->addref();
            return View(view_, flags_ | BOUNDS_ACTIVE, types_, store_, Box(xy), matcher_, filter_);
        }
        throw QueryException("Not yet implented");
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

    View withOther(const View& other) const
    {
        // Cannot combine sets from different stores
        if (store_ != other.store_) return empty();

        FeatureTypes newTypes = types_ & other.types_;
        if (newTypes == 0) return empty();

        if (view_ != WORLD || other.view_ != WORLD)
        {
            // Others will require special handling, rarely used
            // TODO: allow other combinations
            throw QueryException("Not yet implented");
        }

        const Filter* newFilter;
        if (!ComboFilter::tryCombine(&newFilter, filter_, other.filter_))
        {
            return empty();
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
        return { view_, flags_ | other.flags_, newTypes, store_,
            b, newMatcher, newFilter };
    }

    // TODO: Don't use the current matcher; use the allMatcher of the store
    //  (otherwise complex matchers hang around longer than needed)
    View empty() const
    {
        store_->addref();
        matcher_->addref();
        return View(EMPTY, 0, 0, store_, FeaturePtr(), matcher_, nullptr);
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

    uint16_t view_;
    uint16_t flags_;
    FeatureTypes types_;
    FeatureStore* store_;
    const MatcherHolder* matcher_;
    const Filter* filter_;
    Context context_;
};

/// \endcond lowlevel

} // namespace geodesk
