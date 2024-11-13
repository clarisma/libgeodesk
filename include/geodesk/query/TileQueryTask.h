// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/DataPtr.h>
#include <geodesk/query/QueryResults.h>
#include <geodesk/feature/types.h>
#include <geodesk/filter/Filter.h>
#include <geodesk/match/Matcher.h>

namespace geodesk {

class Query;

/// \cond lowlevel
///
class TileQueryTask
{
public:
    TileQueryTask(Query* query, uint32_t tipAndFlags, FastFilterHint fastFilterHint) :
        query_(query),
        tipAndFlags_(tipAndFlags),
        fastFilterHint_(fastFilterHint),     
        results_(QueryResults::EMPTY)
    {
    }

    TileQueryTask() {} // TODO: Never used, exists only to satisfy compiler


    void operator()();

private:
    void searchNodeIndexes();
    void searchNodeBranch(DataPtr p);
    void searchNodeLeaf(DataPtr p);
    void searchIndexes(FeatureIndexType indexType);
    void searchBranch(DataPtr p);
    void searchLeaf(DataPtr p);
    void addResult(uint32_t item);

    Query* query_;
    uint32_t tipAndFlags_;
    FastFilterHint fastFilterHint_;
    DataPtr pTile_;
    QueryResults* results_;
};

// \endcond


} // namespace geodesk
