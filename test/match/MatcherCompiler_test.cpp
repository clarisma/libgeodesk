// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <catch2/catch_test_macros.hpp>
#include <geodesk/match/MatcherCompiler.h>
#include <clarisma/cli/ConsoleWriter.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/geodesk.h>

using namespace geodesk;

TEST_CASE("Multiple negative clauses")
{
    clarisma::Console console;
    FeatureStore* store = FeatureStore::openSingle(
        "d:\\geodesk\\tests\\berlin.gol");
    MatcherCompiler mc(store);
    const MatcherHolder* matcher = mc.getMatcher(
        "a[amenity=parking][access!=no,private,employees,staff,permit,military,agricultural,restricted,delivery][parking!=carports,half_on_kerb,half_on_shoulder,layby,left,on_kerb,sheds,shoulder,lane,street_side]");
    //clarisma::ConsoleWriter out;
    //matcher->explain(out);
}


TEST_CASE("Issue 29")
{
    Features berlin("d:\\geodesk\\tests\\berlin.gol");

    for (auto f : berlin(
        // "a[amenity=parking][access!=private][parking!=street_side]"))
        "a[amenity=parking][access!=no,private,employees,staff,permit,military,agricultural,restricted,delivery][parking!=carports,half_on_kerb,half_on_shoulder,layby,left,on_kerb,sheds,shoulder,lane,street_side]"))
    {
        /*
        if (f["parking"] == "street_side")
        {
            std::cout << f << std::endl;
        }
        */
        std::cout << f["parking"] << std::endl;
    }
}
