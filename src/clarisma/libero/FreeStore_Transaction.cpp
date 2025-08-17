// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/libero/FreeStore_Transaction.h>
#include <clarisma/util/Crc32.h>
#include <cassert>

#include "clarisma/io/MemoryMapping.h"

namespace clarisma {

void FreeStore::Transaction::stageBlock(uint64_t ofs, const void* content)
{
    assert ((ofs & (BLOCK_SIZE - 1)) == 0);
    assert (content);

    auto [it, inserted] = stagedBlocks_.insert(ofs);
    if (!inserted) return;

    journalBuffer_.write(&ofs, sizeof(ofs));
    journalBuffer_.write(content, BLOCK_SIZE);
    journalChecksum_.update(&ofs, sizeof(ofs));
    journalChecksum_.update(content, BLOCK_SIZE);
}

void FreeStore::Transaction::save()
{
    uint64_t trailer = JOURNAL_END_MARKER_FLAG;
    // TODO: DAF
    journalBuffer_.write(&trailer, sizeof(trailer));
    journalChecksum_.update(&trailer, sizeof(trailer));
    uint64_t checksum = journalChecksum_.get();
    journalBuffer_.write(&checksum, sizeof(checksum));
    journalBuffer_.flush();
    journalBuffer_.fileHandle().syncData();
    journalChecksum_ = Crc32();
    stagedBlocks_.clear();

    // TODO: write journal header here?
    // TODO: what if client never calls commit() after save()?
}

uint32_t FreeStore::Transaction::allocPages(uint32_t requestedPages)
{
    assert(requestedPages > 0);
    assert(requestedPages <= (SEGMENT_LENGTH >> store_.pageSizeShift_));

    // checkFreeTrees();
    // LOGS << "Allocating " << requestedPages << " pages\n";

    auto it = freeBySize_.lower_bound(
        static_cast<uint64_t>(requestedPages) << 32);

    if (it != freeBySize_.end())
    {
        // Found a free blob that is large enough

        uint64_t sizeEntry = *it;
        freeBySize_.erase(it);
        uint32_t freePages = static_cast<uint32_t>(sizeEntry >> 32);
        uint32_t firstPage = static_cast<uint32_t>(sizeEntry);

        it = freeByStart_.lower_bound(static_cast<uint64_t>(firstPage) << 32);
        if (it == freeByStart_.end() ||
            (*it >> 32) != firstPage ||
            (static_cast<uint32_t>(*it) >> 1) != freePages)
        {
            // There must be an entry in the free-by-end index with
            // the exact key, TODO: error handling needed?

            assert(false);  // Tree corrupted
        }

        if (freePages == requestedPages)
        {
            /*
            LOGS << "  Found perfect fit of " << requestedPages
                << " pages at " << firstPage;
            */

            // Perfect fit
            freeByStart_.erase(it);
            return firstPage;
        }

        // we need to give back the remaining chunk

        /*
        LOGS << "  Allocated " << requestedPages
            << " pages at " << firstPage << ", giving back "
            << (freePages - requestedPages) << " at " <<
                (firstPage + requestedPages) << "\n";
        */

        bool garbageFlag = static_cast<bool>(*it & 1);
        auto hint = std::next(it);
        freeByStart_.erase(it);
        freeByStart_.insert(hint, (static_cast<uint64_t>(
            firstPage + requestedPages) << 32) |
            ((freePages - requestedPages) << 1) |
            garbageFlag);

        return firstPage;
    }

    uint32_t firstPage = totalPageCount_;
    uint32_t pagesPerSegment = SEGMENT_LENGTH >> store_.pageSizeShift_;
    int remainingPages = pagesPerSegment - (firstPage & (pagesPerSegment - 1));
    if (remainingPages < requestedPages)
    {
        // If the blob won't fit into the current segment, we'll
        // mark the remaining space as a free blob, and allocate
        // the blob in a fresh segment

        uint32_t firstRemainingPage = totalPageCount_;
        firstPage = firstRemainingPage + remainingPages;

        freeBySize_.insert(
            (static_cast<uint64_t>(remainingPages) << 32) |
            firstRemainingPage);
        freeByStart_.insert(
            (static_cast<uint64_t>(firstRemainingPage) << 32) |
            (remainingPages << 1));

        /*
        LOGS << "  Allocated virgin " << requestedPages << " pages at "
            << firstPage << ", skipping " << remainingPages << " at "
            << firstRemainingPage;
        */
        return firstPage;
    }

    /*
    LOGS << "  Allocated virgin " << requestedPages
        << " pages at " << firstPage;
    */

    totalPageCount_ = firstPage + requestedPages;
    return firstPage;
}

} // namespace clarisma
