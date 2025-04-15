// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/query/Query.h>
#include <clarisma/util/log.h>
#include <geodesk/query/TileQueryTask.h>

namespace geodesk {

// #include <boost/asio.hpp>


Query::Query(FeatureStore* store, const Box& box, FeatureTypes types,
    const MatcherHolder* matcher, const Filter* filter) :
    AbstractQuery(store),
    types_(types),
    matcher_(matcher),
    filter_(filter),
    pendingTiles_(0),
    currentResults_(QueryResults::EMPTY),
    currentPos_(QueryResults::EMPTY->count),
    allTilesRequested_(false),
    tileIndexWalker_(store->tileIndex(), store->zoomLevels(), box, filter),
    queuedResults_(QueryResults::EMPTY),
    completedTiles_(0)
{
    /*
    // Don't add refcount to store, wrapper object is responsible for liveness
    // of all resources
    store->addref();        // TODO: should we add a refcount to the store here?
                            // maybe it makes more sense for PyQuery to hold a ref 
                            // to its PyFeatures; this way, store, matcher and filter
                            // are guaranteed to be kept alive for duration of the
                            // query's lifetime
    */
    tileIndexWalker_.next();
        // move the TIW to the root tile (This is not needed in v2,
        // since next() is called *after* each tile, not before)
    requestTiles();
}



// TODO: use a flag that indicates that query finished, which makes the destructor
//  more efficient
Query::~Query()
{
    // LOG("Destroying Query...");
    while(pendingTiles_)
    {
        deleteResults(take());
    }
    deleteResults(currentResults_);
    // LOG("Destroyed Query.");
}


void Query::deleteResults(const QueryResults* res)
{
    while(res != QueryResults::EMPTY)
    {
        QueryResults* next = res->next;
        delete res;
        res = next;
    }
}


void Query::offer(QueryResults* res)
{
    // LOG("Putting fresh results into the queue...");
    std::unique_lock lock(mutex_);
    if (queuedResults_ == QueryResults::EMPTY)
    {
        queuedResults_ = res;
    }
    else if(res != QueryResults::EMPTY)
    {
        QueryResults* first = queuedResults_->next;
        queuedResults_->next = res->next;
        res->next = first;
        queuedResults_ = res;
    }

    completedTiles_++;
    resultsReady_.notify_one();
}

void Query::cancel()
{
    std::unique_lock lock(mutex_);
    // TODO
}

const QueryResults* Query::take()
{
    // LOG("Taking next batch...");
    std::unique_lock lock(mutex_);
    while (completedTiles_ == 0)
    {
        resultsReady_.wait(lock);
    }
    QueryResults* res = queuedResults_;
    queuedResults_ = QueryResults::EMPTY;
    pendingTiles_ -= completedTiles_;
    completedTiles_ = 0;

    // Turn the circular list into a simple list ending with EMPTY
    QueryResults* first = res->next;
    res->next = QueryResults::EMPTY;
    return first;
}

void Query::requestTiles()
{

    // Fill the queue with requests. If the queue is full, submit 1 request
    // (blocking until a spot frees up).
    // TODO: This only works as long as there is only one consumer thread
    // (For Python, this is currently guaranteed)
    // If we have multiple consumer threads, we'll have to submit in a non-blocking
    // fashion, deferring a task that was not accepted. But we always have to
    // submit at least one task, even if it means blocking, so next() will
    // work properly

    /*
    int submitCount = std::max(store_->executor().minimumRemainingCapacity(), 1);
    while (submitCount > 0)
    {
        if (!tileIndexWalker_.next())
        {
            allTilesRequested_ = true;
            break;
        }
        TileQueryTask task(this,
            (tileIndexWalker_.currentTip() << 8) |
            tileIndexWalker_.northwestFlags(),
            FastFilterHint(tileIndexWalker_.turboFlags(), tileIndexWalker_.currentTile()));
        store_->executor().post(task);
        pendingTiles_++;
        submitCount--;
    }
    */

    bool postedAny = false;
    for (;;)
    {
        TileQueryTask task(this,
            (tileIndexWalker_.currentTip() << 8) |
            tileIndexWalker_.northwestFlags(),
            FastFilterHint(tileIndexWalker_.turboFlags(), tileIndexWalker_.currentTile()));

        // LOG("Trying to submit %06X...", tileIndexWalker_.currentTip());

        if (!store_->executor().tryPost(task))
        {
            // If the queue is full and we haven't been able to
            // post at least one task, we'll run the task on the main
            // thread; otherwise, we'll end up waiting for a tile
            // that will never arrive = deadlock

            if (postedAny)  [[likely]]
            {
                break;
            }
            pendingTiles_++;
            // LOG("Running %06X on main thread...", tileIndexWalker_.currentTip());
            task();
        }
        else
        {
            pendingTiles_++;
            // LOG("  Submitted %06X", tileIndexWalker_.currentTip());
        }
        postedAny = true;
        if (!tileIndexWalker_.next())
        {
            // LOG("All tiles submitted.");
            allTilesRequested_ = true;
            break;
        }
    }
    
}

FeaturePtr Query::next()
{
    for (;;)
    {
        if (currentPos_ == currentResults_->count)
        {
            // We're at the end of the current batch;
            // move on to the next
            QueryResults* next = currentResults_->next;
            if (currentResults_ != QueryResults::EMPTY) delete currentResults_;
            currentPos_ = 0;
            currentResults_ = next;
            if (next == QueryResults::EMPTY)
            {
                // We've consumed all current batches;
                for(;;)
                {
                    if (pendingTiles_ == 0)
                    {
                        // There are no more tiles: We're done
                        return nullptr;
                    }
                    const QueryResults* res = take();
                    if (!allTilesRequested_) requestTiles();
                    if (res != QueryResults::EMPTY)
                    {
                        currentResults_ = res;
                        break;
                    }
                }
            }
            else
            {
                // LOG("Next batch of current results...");
            }
        }
        uint32_t item = currentResults_->items[currentPos_++];
        DataPtr pTile = currentResults_->pTile;
        if (item & REQUIRES_DEDUP)
        {
            FeaturePtr pFeature (pTile + (item & ~REQUIRES_DEDUP));
            uint64_t idBits = pFeature.idBits();  // getUnsignedLong() & 0xffff'ffff'ffff'ff18LL;
            if (potentialDupes_.count(idBits)) continue;
            potentialDupes_.insert(idBits);
            return pFeature;
        }
        return FeaturePtr(pTile + item);
    }
}


} // namespace geodesk
