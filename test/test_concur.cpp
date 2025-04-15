#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <catch2/catch_test_macros.hpp>
#include <geodesk/geodesk.h>

using namespace geodesk;

// Structure to hold test information
struct GeodeskConcurTest
{
    GeodeskConcurTest(const std::string& n, std::function<int64_t()> f) :
        name(n), function(f) {}

    std::string name;
    std::function<int64_t()> function;
};

// Vector to hold all the tests
std::vector<GeodeskConcurTest> geodesk_concur_tests;

// Function to register a test
void registerConcurTest(const std::string& name, std::function<int64_t()> func)
{
    geodesk_concur_tests.emplace_back(name, func);
}

// Macro to define a test
#define GEODESK_TEST(test_name)                                  \
int64_t test_name##_impl();                                      \
struct test_name##_registrar                                     \
{                                                                \
    test_name##_registrar()                                      \
    {                                                            \
        registerConcurTest(#test_name, test_name##_impl);        \
    }                                                            \
} test_name##_registrar_instance;                                \
int64_t test_name##_impl()

static Features world(R"(c:\geodesk\tests\monaco.gol)");

Feature findLargestCountry()
{
    static FeaturePtr largestCountry;

    if(largestCountry.isNull())
    {
        std::vector<std::pair<double,FeaturePtr>> countries;
        for(auto country: world("a[boundary=administrative][admin_level=2]"))
        {
            countries.emplace_back(country.area(), country.ptr());
        }
        std::sort(countries.begin(), countries.end());
        largestCountry = countries.back().second;
        assert(!largestCountry.isNull());
    }
    return {world.store(), largestCountry};
}

/*
Feature findLongestRiver()
{
    static FeaturePtr longestRiver;

    if(longestRiver.isNull())
    {
        std::vector<std::pair<double,FeaturePtr>> rivers;
        for(auto river: world("r[waterway=river]"))
        {
            rivers.emplace_back(river.length(), river.ptr());
        }
        std::sort(rivers.begin(), rivers.end());
        longestRiver = rivers.back().second;
        assert(!biggestCountry.isNull());
    }
    return {world.store(), longestRiver};
}
*/

GEODESK_TEST(areas_containing_largest_country_centroid_count)
{
    return world("a").containing(findLargestCountry().centroid()).count();
}

GEODESK_TEST(centroid_hash)
{
    int64_t hash = 0;
    for (auto f: world)
    {
        Coordinate c = f.centroid();
        hash ^= c.x;
        hash ^= c.y;
    }
    return hash;
}

GEODESK_TEST(id_hash)
{
    int64_t hash = 0;
    for (auto f: world)
    {
        hash ^= f.id();
    }
    return hash;
}

GEODESK_TEST(italian_restaurant_count)
{
    return world("na[amenity=restaurant][cuisine=italian]").count();
}

/*
GEODESK_TEST(largest_country_id)
{
    return findLargestCountry().id();
}

GEODESK_TEST(largest_country_intersects_count)
{
    return world.intersecting(findLargestCountry()).count();
}

GEODESK_TEST(max_1km_from_largest_country_centroid_count)
{
    return world.maxMetersFrom(1000,
        findLargestCountry().centroid()).count();
}
*/

GEODESK_TEST(member_count)
{
    int64_t count = 0;
    for (auto parent: world)
    {
        count += parent.members().count();
    }
    return count;
}

GEODESK_TEST(member_iter_count)
{
    int64_t count = 0;
    for (auto parent: world)
    {
        for(auto child: parent.members()) count++;
    }
    return count;
}

GEODESK_TEST(nonsense_parent_count)
{
    int64_t count = 0;
    for (auto child : world)
    {
        count += child.parents().nodes().count();
    }
    return count;
}

GEODESK_TEST(nonsense_parents_of_count)
{
    auto nodes = world.nodes();
    int64_t count = 0;
    for (auto child : world)
    {
        count += nodes.parentsOf(child).count();
    }
    return count;
}

GEODESK_TEST(parent_count)
{
    int64_t count = 0;
    for (auto child : world)
    {
        count += child.parents().count();
    }
    return count;
}

GEODESK_TEST(parent_iter_count)
{
    int64_t count = 0;
    for (auto child : world)
    {
        for (auto parent : child.parents())
        {
            count += 1;
        }
    }
    return count;
}

GEODESK_TEST(parents_of_count)
{
    int64_t count = 0;
    for (auto child : world)
    {
        count += world.parentsOf(child).count();
    }
    return count;
}

GEODESK_TEST(parent_relations_count)
{
    int64_t count = 0;
    for (auto child : world)
    {
        count += child.parents().relations().count();
    }
    return count;
}

GEODESK_TEST(parent_relations_of_count)
{
    Features relations = world.relations();
    int64_t count = 0;
    for (auto child : world)
    {
        count += relations.parentsOf(child).count();
    }
    return count;
}

GEODESK_TEST(parent_ways_count)
{
    int64_t count = 0;
    for (auto child : world)
    {
        count += child.parents().ways().count();
    }
    return count;
}

GEODESK_TEST(parent_ways_of_count)
{
    auto ways = world.ways();
    int64_t count = 0;
    for (auto child : world)
    {
        count += ways.parentsOf(child).count();
    }
    return count;
}

GEODESK_TEST(relation_member_role_len)
{
    int64_t len = 0;
    for (auto parent: world.relations())
    {
        for(auto child: parent.members())
        {
            // std::cout << parent << ": " << child << " as " << child.role() << std::endl;
            len += child.role().size();
        }
    }
    return len;
}

GEODESK_TEST(street_crossing_count)
{
    Nodes crossings =  world("n[highway=crossing]");
    int64_t count = 0;
    for (auto street: world("w[highway]"))
    {
        count += crossings.nodesOf(street).count();
    }
    return count;
}

GEODESK_TEST(street_crossing_iter_count)
{
    Nodes crossings =  world("n[highway=crossing]");
    int64_t count = 0;
    for (auto street: world("w[highway]"))
    {
        for(auto node: crossings.nodesOf(street)) count++;
    }
    return count;
}

GEODESK_TEST(tags_count)
{
    int64_t count = 0;
    for (auto f: world)
    {
        count += f.tags().size();
    }
    return count;
}

GEODESK_TEST(tags_iter_count)
{
    int64_t count = 0;
    for (auto f: world)
    {
        for(auto tag: f.tags()) count++;
    }
    return count;
}

GEODESK_TEST(tags_key_len)
{
    int64_t totalLen = 0;
    for (auto f: world)
    {
        for(auto tag: f.tags())
        {
            totalLen += static_cast<int64_t>(tag.key().size());
        }
    }
    return totalLen;
}

GEODESK_TEST(tags_str_len)
{
    int64_t totalLen = 0;
    for (auto f: world)
    {
        for(auto tag: f.tags())
        {
            std::string strValue = tag.value();
            totalLen += static_cast<int64_t>(strValue.size());
        }
    }
    return totalLen;
}

GEODESK_TEST(tags_int_sum)
{
    int64_t sum = 0;
    for (auto f: world)
    {
        for(auto tag: f.tags())
        {
            sum += static_cast<int>(tag.value());
        }
    }
    return sum;
}

GEODESK_TEST(waynode_count)
{
    int64_t count = 0;
    for (auto way: world.ways())
    {
        count += way.nodes().count();
    }
    return count;
}

GEODESK_TEST(waynode_iter_count)
{
    int64_t count = 0;
    for (auto way: world.ways())
    {
        for(auto node: way.nodes()) count++;
    }
    return count;
}

GEODESK_TEST(waynode_parents_count)
{
    int64_t count = 0;
    for (auto way : world.ways())
    {
        for (auto node : way.nodes())
        {
            count += node.parents().count();
        }
    }
    return count;
}

GEODESK_TEST(waynode_parent_ways_count)
{
    int64_t count = 0;
    for (auto way : world.ways())
    {
        for (auto node : way.nodes())
        {
            count += node.parents().ways().count();
        }
    }
    return count;
}

GEODESK_TEST(xy_hash)
{
    int64_t hash = 0;
    for (auto f: world)
    {
        hash ^= f.x();
        hash ^= f.y();
    }
    return hash;
}

/*
GEODESK_TEST(lonlat_100nd_hash)
{
    int64_t hash = 0;
    for (auto f: world)
    {
        // hash ^= (long)(f.lon() * 10000000);
        // hash ^= (long)(f.lat() * 10000000);
        hash ^= (long)(Mercator.lonPrecision7fromX(f.x()) * 10000000);
        hash ^= (long)(Mercator.latPrecision7fromY(f.y()) * 10000000);
    }
    return hash;
}
 */




TEST_CASE("concur")
{
    for(auto test: geodesk_concur_tests)
    {
        std::cout << test.name << "=" << test.function() << std::endl;
    }
}