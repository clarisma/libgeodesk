// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/geom/Box.h>
#include <geodesk/feature/FeaturePtr.h>

namespace geodesk {

class FeatureStore;
class Filter;
class MatcherHolder;

/// \cond lowlevel

class Selection
{
protected:
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

    Selection(int kind, int flags, FeatureTypes types, const FeatureStore* store,
        Context context, const MatcherHolder* matcher, const Filter* filter) :
        kind_(kind), flags_(flags), types_(types), store_(store),
        context_(context), matcher_(matcher), filter_(filter)
    {
    }

    uint32_t kind() const noexcept { return kind_; }
    const FeatureStore* store() const noexcept { return store_; }
    FeatureTypes types() const noexcept { return types_; }
    const MatcherHolder* matcher() const noexcept { return matcher_; }
    const Filter* filter() const { return filter_; }
    Box bounds() const noexcept
    {
        assert(flags_ & USES_BOUNDS);
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

    [[nodiscard]] GEODESK_API bool isEmpty() const;
    [[nodiscard]] GEODESK_API uint64_t count() const;

protected:
    bool contains(FeaturePtr feature) const;
    bool containsAnonNode(Coordinate xy) const;

    uint16_t kind_;
    uint16_t flags_;
    FeatureTypes types_;
    const FeatureStore* store_;
    const MatcherHolder* matcher_;
    const Filter* filter_;
    Context context_;
};

/// \endcond lowlevel

} // namespace geodesk
