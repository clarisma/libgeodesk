// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <condition_variable>
#include <geodesk/query/QueryResults.h>
#include <geodesk/query/TileIndexWalker.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/geom/Box.h>

namespace geodesk {

class Filter;

// TODO: Maybe call this a "Cursor"

// TODO: Where to put bounds and filter?
//  These are used by TIW, but are also part of the Query descriptor
//  (need store, box, matcher, filter and type)

// TODO: The fields of this class may suffer form false sharing,
//  as they are read by the query worker threads
//  - currentPos_ is written only be the main thread
//  - The TIW contains box, but also has fields that are written
//    by the main thread as the tiles are iterated
//  - may make sense to duplicate the bbox in Query, so the workers
//    don't read it from the TIW, where it may sit in the same cache line
//    as the fields to which the TIW writes.
//  It might even make sense to merge Query and TIW into a single class,
//   and rearrange the fields there
//
// Field            Size    Access      User
// bounds           16      R           TIW, workers
// store            8       R           Query, workers
// types            4       R           workers
// matcher          8       R           workers
// filter           8       R           TIW, workers
// consumer         8       R           workers
// pIndex           8       R           TIW
// currentLevel     4       R/W         TIW
// currentTile      4       R/W         TIW
// currentTip_      4       R/W         TIW
// northwestFlags   4       R/W         TIW
// turboFlags_      4       R/W         TIW
// tileBasedAcc     1       R           TIW
// trackAccepted    1       R           TIW
//
// Or, we could simply use a pointer to a View, where these fields
// are already present


class Query
{
public:
    Query(FeatureStore* store, const Box& box, FeatureTypes types, 
        const MatcherHolder* matcher, const Filter* filter); 
    ~Query();
    const Box& bounds() const { return tileIndexWalker_.bounds(); }
    FeatureTypes types() const { return types_; }
    const MatcherHolder* matcher() const { return matcher_; }
    const Filter* filter() const { return filter_; }
    FeatureStore* store() const { return store_; }
    void offer(QueryResults* results);
    void cancel();

    FeaturePtr next();

    // static constexpr uint32_t REQUIRES_DEDUP = 0x8000'0000;

private:
    const QueryResults* take();
    void requestTiles();
    static void deleteResults(const QueryResults* res);

    FeatureStore* store_;
    FeatureTypes types_;
    const MatcherHolder* matcher_;
    const Filter* filter_;
    int32_t pendingTiles_;      // TODO: rearrange to avoid needless gaps
    const QueryResults* currentResults_;
    int32_t currentPos_;
    bool allTilesRequested_;
    TileIndexWalker tileIndexWalker_;

    // these are used by multiple threads:
    // TODO: padding to avoid false sharing
    // maybe use TileIndexWalker for padding as it has lots
    // of unused entries?

    std::mutex mutex_;
    std::condition_variable resultsReady_;  // requires mutex_
    QueryResults* queuedResults_;           // requires mutex_
    int32_t completedTiles_;                // requires mutex_
    // bool isCancelled_;                      // requires mutex_
};


} // namespace geodesk
