// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include "../tests.h"
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/match/Matcher.h>

using namespace geodesk;

// Issue #55: Use tag index regardless of clause order

namespace {

uint32_t indexBitsOf(FeatureStore* store, const char* key)
{
    int code = store->strings().getCode(key, strlen(key));
    REQUIRE(code > 0);
    int category = store->snapshot()->getIndexCategory(code);
    REQUIRE(category > 0);      // key must be indexed for this test
    return IndexBits::fromCategory(category);
}

// Checks that two queries accept exactly the same index buckets
void requireSameIndexUse(FeatureStore* store, const char* a, const char* b)
{
    const MatcherHolder* ma = store->getMatcher(a);
    const MatcherHolder* mb = store->getMatcher(b);
    for (int index = 0; index < 4; index++)
    {
        for (uint32_t keys = 0; keys < (1u << 16); keys++)
        {
            INFO("query A: " << a << "\nquery B: " << b
                << "\nindex=" << index << " keys=" << keys);
            REQUIRE(ma->acceptIndex(static_cast<FeatureIndexType>(index), keys) ==
                mb->acceptIndex(static_cast<FeatureIndexType>(index), keys));
        }
    }
    ma->release();
    mb->release();
}

} // namespace

TEST_CASE("Issue 55: tag index is used regardless of clause order")
{
    Features monaco = getMonaco();
    FeatureStore* store = monaco.store();
    uint32_t boundaryBits = indexBitsOf(store, "boundary");

    SECTION("unindexed clause before indexed clause")
    {
        const MatcherHolder* m = store->getMatcher(
            "a[name=Monaco][boundary=administrative]");
        // A bucket without the "boundary" key must be skipped
        CHECK_FALSE(m->acceptIndex(static_cast<FeatureIndexType>(2), 0));
        CHECK(m->acceptIndex(static_cast<FeatureIndexType>(2), boundaryBits));
        m->release();

        requireSameIndexUse(store,
            "a[name=Monaco][boundary=administrative]",
            "a[boundary=administrative][name=Monaco]");
    }

    SECTION("negated clause merged with key-required clause")
    {
        requireSameIndexUse(store,
            "a[boundary!=political][boundary]",
            "a[boundary][boundary!=political]");
    }

    SECTION("type-only selector disables index filtering")
    {
        // The "n" selector matches nodes in any bucket, so the index
        // must not be used to skip buckets
        const MatcherHolder* m = store->getMatcher("n, a[boundary]");
        CHECK(m->acceptIndex(static_cast<FeatureIndexType>(0), 0));
        m->release();
    }

    SECTION("query results do not depend on clause order")
    {
        CHECK(monaco("a[name=Monaco][boundary=administrative]").count() ==
            monaco("a[boundary=administrative][name=Monaco]").count());
        CHECK(monaco("a[boundary=administrative][name=Monaco]").count() > 0);
        CHECK(monaco("n, a[boundary]").count() ==
            monaco("n").count() + monaco("a[boundary]").count());
    }
}
