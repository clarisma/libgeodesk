// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/libero/FreeStore_Transaction.h>
#include <clarisma/util/Crc32.h>
#include <cassert>

#include "clarisma/io/MemoryMapping.h"
#include "clarisma/util/log.h"

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
            --freeRangeCount_;
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
        ++freeRangeCount_;

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


void FreeStore::Transaction::freePages(uint32_t firstPage, uint32_t pages)
{
    assert(pages > 0);
    assert(pages <= SEGMENT_LENGTH >> store_.pageSizeShift_);

    // checkFreeTrees();
    // LOGS << firstPage << ": Freeing " << pages << " pages\n";

    if (firstPage + pages == totalPageCount_)
    {
        // Blob is at end -> trim the file
        totalPageCount_ -= pages;
        while (!freeByStart_.empty())
        {
            auto it = std::prev(freeByStart_.end());
            uint32_t first = static_cast<uint32_t>(*it >> 32);
            uint32_t size = static_cast<uint32_t>(*it) >> 1;
            if (first + size != totalPageCount_) break;
            totalPageCount_ = first;
            uint64_t sizeKey =
                (static_cast<uint64_t>(size) << 32) | first;
            auto itSize = freeBySize_.lower_bound(sizeKey);
            assert(itSize != freeBySize_.end() && *itSize == sizeKey);
            freeBySize_.erase(itSize);
            freeByStart_.erase(it);
            --freeRangeCount_;

            // LOGS << first << ": Trimmed " << size << " from end";
        }
        return;
    }

    auto right = freeByStart_.lower_bound(
    static_cast<uint64_t>(firstPage) << 32);
    if (right != freeByStart_.end())
    {
        uint32_t rightStart = static_cast<uint32_t>(*right >> 32);
        if (rightStart == firstPage + pages &&
            !isFirstPageOfSegment(rightStart))
        {
            // coalesce with right neighbor
            uint32_t rightSize = static_cast<uint32_t>(*right) >> 1;
            pages += rightSize;
            auto next = std::next(right);
            freeByStart_.erase(right);
            auto itSize = freeBySize_.lower_bound(
                (static_cast<uint64_t>(rightSize) << 32) | rightStart);
            if (itSize == freeBySize_.end() ||
                (*itSize >> 32) != rightSize ||
                static_cast<uint32_t>(*itSize) != rightStart)
            {
                assert(false);
            }
            freeBySize_.erase(itSize);
            --freeRangeCount_;
            right = next;
        }
    }
    if (!freeByStart_.empty() && right != freeByStart_.begin())
    {
        auto left = std::prev(right);
        uint32_t leftStart = static_cast<uint32_t>(*left >> 32);
        uint32_t leftSize = static_cast<uint32_t>(*left) >> 1;
        if (leftStart + leftSize == firstPage &&
            !isFirstPageOfSegment(firstPage))
        {
            // coalesce with left neighbor
            firstPage = leftStart;
            pages += leftSize;
            freeByStart_.erase(left);
            auto itSize = freeBySize_.lower_bound(
                (static_cast<uint64_t>(leftSize) << 32) | leftStart);
            if (itSize == freeBySize_.end() ||
                (*itSize >> 32) != leftSize ||
                static_cast<uint32_t>(*itSize) != leftStart)
            {
                assert(false);
            }
            freeBySize_.erase(itSize);
            --freeRangeCount_;
        }
    }
    freeByStart_.insert(right,
        (static_cast<uint64_t>(firstPage) << 32) |
        (pages << 1) | 1);
    freeBySize_.insert(
        (static_cast<uint64_t>(pages) << 32) | firstPage);
    ++freeRangeCount_;
}

void FreeStore::Transaction::writeFreeRangeIndex()
{
    if (freeRangeCount_ == 0) [[unlikely]]
    {
        header_.freeRangeIndex = 0;
        return;
    }
    uint32_t indexSize = (freeRangeCount_ + 1) * sizeof(uint64_t);
    uint32_t indexSizeInPages = store_.pagesForBytes(indexSize);
    auto it = freeBySize_.lower_bound(indexSizeInPages);
    uint32_t indexPage;
    if (it != freeBySize_.end()) [[likely]]
    {
        indexPage = static_cast<uint32_t>(*it >> 32);
    }
    else
    {
        indexPage = totalPageCount_;
        totalPageCount_ += indexSizeInPages;
        // TODO: clarify if the FRI is allowed to straddle
        //  segment boundaries (it is not used by consumers
        //  that may require segment-based mapping)
    }
    std::unique_ptr<uint64_t[]> index(new uint64_t[freeRangeCount_ + 1]);
    uint64_t* p = index.get();
    *p++ = (freeRangeCount_ * 8) + 4;
    for (auto entry : freeByStart_)
    {
        *p++ = entry;
    }
    assert(p - index.get() == freeRangeCount_ + 1);

    store_.file_.writeAllAt(indexPage << store_.pageSizeShift_,
        index.get(), indexSize);
    header_.freeRangeIndex = indexPage;
}

void FreeStore::Transaction::dumpFreeRanges()
{
    size_t sizeSize = 0;
    size_t sizeCount = 0;
    LOGS << "Free pages by size:\n";
    for (auto entry : freeBySize_)
    {
        uint32_t size = static_cast<uint32_t>(entry >> 32);
        LOGS << "- " << static_cast<uint32_t>(entry) << ": " << size << '\n';
        sizeCount++;
        sizeSize += size;
    }
    LOGS << "  " << sizeCount << " entries with " << sizeSize << " total pages\n";

    size_t startSize = 0;
    size_t startCount = 0;

    LOGS << "Free pages by location:\n";
    for (auto entry : freeByStart_)
    {
        uint32_t size = static_cast<uint32_t>(entry) >> 1;
        LOGS << "- " << static_cast<uint32_t>(entry >> 32) << ": " << size << '\n';
        startCount++;
        startSize += size;
    }
    LOGS << "  " << startCount << " entries with " << startSize << " total pages\n";
    LOGS << totalPageCount_ << " total pages\n";
    LOGS << "Free ratio: " << (static_cast<double>(startSize) / totalPageCount_) << "\n";
    assert(startCount == sizeCount);
    assert(startSize == sizeSize);
}

} // namespace clarisma
