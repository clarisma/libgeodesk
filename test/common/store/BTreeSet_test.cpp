// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only


#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <string_view>
#include <catch2/catch_test_macros.hpp>
#include <clarisma/store/KeyTree.h>
#include <clarisma/data/HashMap.h>
#include <clarisma/data/HashSet.h>
#include <gtl/btree.hpp>


using namespace clarisma;

template<typename K>
class BTreeSet : public gtl::btree_set<K>
{
public:
    K takeLowerBound(K k)
    {
        auto it = this->lower_bound(k);
        if (it == this->end()) return K();
        k = *it;
        this->erase(it);
        return k;
    }
};


TEST_CASE("Random BTreeSet")
{
    BTreeSet<uint64_t> tree;

    // one-time set-up, e.g. in main()
    std::random_device rd;                         // nondet seed
    std::mt19937_64    rng{rd()};                  // 64-bit Mersenne Twister
    std::uniform_int_distribution<uint64_t> dist(1, 2'000'000'000); // inclusive bounds

    int targetAttemptCount = 1000000;
    int targetCount = 0;
    uint32_t hash = 0;

    for (int i=0; i<targetAttemptCount; i++)
    {
        uint32_t k = dist(rng);
        // std::cout << "Inserting # << " << i << ": " << k << std::endl;
        auto [it, inserted] = tree.insert(k);
        targetCount += inserted;
        // tree.check();
        hash ^= inserted ? k : 0;
    }

    int actualCount = 0;
    uint32_t actualHash = 0;
    for (auto k : tree)
    {
        ++actualCount;
        actualHash ^= k;
        // std::cout << k << " = " << v << std::endl;
    }

    // tree.check();
    REQUIRE(actualCount == targetCount);
    REQUIRE(actualHash == hash);

    int removeCount = targetCount / 2;
    for (int i=0; i<removeCount; i++)
    {
        uint32_t k = dist(rng);
        auto ek = tree.takeLowerBound(k);
        if (ek)
        {
            REQUIRE(ek >= k);
            hash ^= ek;
            --targetCount;
            /*
            size_t currentCount = tree.size();
            printf("Items in tree: %zu\n", currentCount);
            REQUIRE(currentCount == targetCount);
            fflush(stdout);
            tree.check();
            */
        }
    }

    actualCount = 0;
    actualHash = 0;
    for (auto k : tree)
    {
        ++actualCount;
        actualHash ^= k;
    }

    // tree.check();
    REQUIRE(actualCount == targetCount);
    REQUIRE(actualHash == hash);


    /*
    for (int i=0; i<actualCount; i++)
    {
        tree.removeLast(&tx);
    }

    it = tree.iter(&tx);
    REQUIRE(!it.hasNext());
    */

}

