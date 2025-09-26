// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/DataPtr.h>
#include <geodesk/query/QueryResults.h>
#include <geodesk/feature/types.h>
#include <geodesk/filter/Filter.h>
#include <geodesk/match/Matcher.h>

namespace geodesk {

class QueryBase;

/// \cond lowlevel
///
/// Instead of passing a pointer to the tile, we could let the task
/// retrieve it based on the TIP; however, this would require accessing
/// the Tile Index, which may not be in the cache of the core that is
/// executing the task.
///
class TileQueryTask
{
public:
    TileQueryTask(QueryBase* query, uint32_t tipAndFlags, FastFilterHint fastFilterHint) :
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

    QueryBase* query_;
    uint32_t tipAndFlags_;
    FastFilterHint fastFilterHint_;
    DataPtr pTile_;
    QueryResults* results_;
};

// \endcond


} // namespace geodesk
