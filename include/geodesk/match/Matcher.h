// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <cstdint>
#include <geodesk/feature/types.h>
#include <geodesk/feature/FeaturePtr.h>

namespace geodesk {

class FeatureStore;
class Matcher;
class MatcherHolder;
class RoleMatcher;

/// \cond lowlevel

typedef bool (*MatcherMethod)(const Matcher*, FeaturePtr);
typedef const Matcher* (*RoleMatcherMethod)(const RoleMatcher*, FeaturePtr);

// MatcherHolder is a variable-length structure that bundles one or more Matchers,
// a RoleMatcher and their associated resources (pointers to other MatcherHolder
// objects, Regex patterns, string constants, numeric constants)
// Resources are placed ahead of the MatcherHolder, so we need to take care when
// deallocating, because the pointer may not point to the actual start of the 
// heap object.
//
// <resources>              (variable length)
//   <pointers to other MatcherHolders>
//   <Regex patterns>
//   <strings & numeric constants>
// MatcherHolder            <-- pointer points here
//   Header
//   Default role Matcher
//   Main Matcher           (length may vary based on type)



class Matcher
{
public:
    Matcher(MatcherMethod func, FeatureStore* store) : function_(func), store_(store) {}

    // TODO: Can this throw?
    bool accept(FeaturePtr feature) const
    {
        return function_(this, feature);
    }

    FeatureStore* store() const { return store_; }
    
private:
    MatcherMethod function_;
    FeatureStore* store_;           // not refcounted
};

class RoleMatcher
{
public:
    RoleMatcher(RoleMatcherMethod func, FeatureStore* store) : function_(func), store_(store) {}

    const Matcher* accept(FeaturePtr feature) const
    {
        return function_(this, feature);
    }

private:
    RoleMatcherMethod function_;
    FeatureStore* store_;           // not refcounted
};


struct IndexMask
{
    uint32_t keyMask;
    uint32_t keyMin;
};

class GEODESK_API MatcherHolder
{
public:
    MatcherHolder() : MatcherHolder(FeatureTypes::ALL, 0xffff'ffff, 0) {}
    MatcherHolder(FeatureTypes types) : MatcherHolder(types, 0xffff'ffff, 0) {}
    MatcherHolder(FeatureTypes types, uint32_t keyMask, uint32_t keyMin);

    static const MatcherHolder* createMatchAll(FeatureTypes types);
    static const MatcherHolder* createMatchKey(FeatureTypes types, 
        uint32_t indexBits, int keyCode, int codeNo);
    static const MatcherHolder* createMatchKeyValue(FeatureTypes types, 
        uint32_t indexBits, int keyCode, int valueCode);

    /**
     * Returns a matcher that must match both a and b.
     * (This function steals the references to a and b, which means
     * the caller is responsible for calling addref() if needed)
     */
    static const MatcherHolder* combine(
        const MatcherHolder* a, const MatcherHolder* b);

    void dealloc() const;
#ifdef GEODESK_MULTITHREADED
    void addref() const
    {
        refcount_.fetch_add(1, std::memory_order_relaxed);
    }

    void release() const
    {
        if (refcount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            dealloc();
        }
    }
#else
    void addref() const { ++refcount_; }
    void release() const { if (--refcount_ == 0) dealloc(); }
#endif

    const Matcher& mainMatcher() const { return mainMatcher_; }
    FeatureTypes acceptedTypes() const { return acceptedTypes_; }

    bool acceptIndex(FeatureIndexType index, uint32_t keys) const
    {
        assert(index >= 0 && index <= 4);
        const IndexMask& mask = indexMasks_[index];
        return ((keys & mask.keyMask) >= mask.keyMin);
    }

private:
    static const Matcher* defaultRoleMethod(const RoleMatcher* matcher, FeaturePtr);
    static bool matchAllMethod(const Matcher*, FeaturePtr);
    static uint8_t* alloc(size_t size) { return new uint8_t[size]; };

#ifdef GEODESK_MULTITHREADED
    mutable std::atomic_uint32_t refcount_;
#else
    mutable uint32_t refcount_;
#endif

    FeatureTypes acceptedTypes_;
    uint32_t resourcesLength_;
    uint32_t referencedMatcherHoldersCount_;
        // number of other MatcherHolder objects referenced
        // Their pointers are stored at the beginning of the <resources>
        // section (see above) and must be managed via addref() and release()
    uint32_t regexCount_;           // number of regexes in resources
    uint32_t roleMatcherOffset_;    // where to find role Matcher
    IndexMask indexMasks_[4];       // one for each: Nodes, Ways, areas, Relations
    RoleMatcher defaultRoleMatcher_;
    Matcher mainMatcher_;

    friend class MatcherCompiler;
    friend class ComboMatcher;
};

// \endcond
} // namespace geodesk
