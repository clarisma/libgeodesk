// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/libero/FreeStore_Transaction.h>
#include <cassert>
#include <cstdio>
#include <random>

#include "clarisma/io/MemoryMapping.h"
#include "clarisma/util/log.h"

namespace clarisma {


void FreeStore::Transaction::begin()
{
    if (store_.created_)  [[unlikely]]
    {
        memset(&header_, 0, sizeof(header_));
    }
    else
    {
        memcpy(&header_, store_.mapping_.data(), sizeof(header_));
        readFreeRangeIndex();
        journal_.open(store_.journalFileName());
    }
}

void FreeStore::Transaction::end()
{
    if (!store_.created_)  [[likely]]
    {
        journal_.close();
        std::remove(store_.journalFileName());
    }
    else
    {
        store_.created_ = false;
        // TODO: check this logic
    }
}

void FreeStore::Transaction::stageBlock(uint64_t ofs, const void* content)
{
    assert ((ofs & (BLOCK_SIZE - 1)) == 0);
    assert (content);

    auto [it, inserted] = editedBlocks_.try_emplace(ofs, content);
    if (!inserted) return;

    journal_.addBlock(ofs, content, BLOCK_SIZE);
}


void FreeStore::Transaction::commit(bool isFinal)
{
    // Actually free the staged free ranges

    for (uint64_t freeRange : stagedFreeRanges_)
    {
        uint32_t firstPage = static_cast<uint32_t>(freeRange >> 32);
        uint32_t pages = static_cast<uint32_t>(freeRange);
        performFreePages(firstPage, pages);
    }
    stagedFreeRanges_.clear();

    if (isFinal)
    {
        // if (header_.freeRangeIndex == INVALID_FREE_RANGE_INDEX)
        // {
        writeFreeRangeIndex();
        // }
    }

    header_.commitId++;
    Crc32C crc;
    crc.update(&header_, CHECKSUMMED_HEADER_SIZE);
    header_.checksum = crc.get();
    // LOGS << "Calculated header checksum: " << header_.checksum;

    if (!store_.created_)   [[likely]]
    {
        journal_.seal();

        for (auto [ofs, content] : editedBlocks_)
        {
            store_.file_.writeAllAt(ofs, content, BLOCK_SIZE);
        }

        journal_.reset(store_.lockedExclusively_ ?
            Journal::MODIFIED_ALL : Journal::MODIFIED_INACTIVE, &header_);
        editedBlocks_.clear();
    }

    store_.file_.syncData();
    store_.file_.writeAllAt(0, &header_, sizeof(header_));
    store_.file_.syncData();
}


uint32_t FreeStore::Transaction::allocPages(uint32_t requestedPages)
{
    assert(requestedPages > 0);
    assert(requestedPages <= (SEGMENT_LENGTH >> store_.pageSizeShift_));

    // header_.freeRangeIndex = INVALID_FREE_RANGE_INDEX;

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

        assert(firstPage + freePages < header_.totalPages);

        it = freeByStart_.lower_bound(static_cast<uint64_t>(firstPage) << 32);
        if (it == freeByStart_.end() ||
            (*it >> 32) != firstPage ||
            (static_cast<uint32_t>(*it) >> 1) != freePages)
        {
            // There must be an entry in the free-by-end index with
            // the exact key, TODO: error handling needed?

            if (it == freeByStart_.end())
            {
                LOGS << "Free-by-start index is missing " << firstPage << ":" << freePages;
            }
            else
            {
                LOGS << "Free-by-start index has wrong size for " << firstPage << ":\n"
                    << " Expected " << freePages << " instead of " << (static_cast<uint32_t>(*it) >> 1);
            }
            assert(false);  // Tree corrupted
        }

        if (freePages == requestedPages)
        {
            //LOGS << "  Found perfect fit of " << requestedPages
            //    << " pages at " << firstPage;

            // Perfect fit
            freeByStart_.erase(it);
            --header_.freeRanges;

            // dumpFreeRanges();

            assert(freeByStart_.size() == header_.freeRanges);
            assert(freeBySize_.size() == header_.freeRanges);
            return firstPage;
        }

        // we need to give back the remaining chunk

        //LOGS << "  Allocated " << requestedPages
        //    << " pages at " << firstPage << ", giving back "
        //    << (freePages - requestedPages) << " at " <<
        //        (firstPage + requestedPages) << "\n";

        bool garbageFlag = static_cast<bool>(*it & 1);
        uint32_t leftoverSize = freePages - requestedPages;
        uint32_t leftoverStart = firstPage + requestedPages;

        it = freeByStart_.erase(it);
        freeByStart_.insert(it,
            (static_cast<uint64_t>(leftoverStart) << 32) |
            (leftoverSize << 1) |
            static_cast<uint64_t>(garbageFlag));

        freeBySize_.insert((static_cast<uint64_t>(leftoverSize) << 32) | leftoverStart);

        assert(freeByStart_.size() == header_.freeRanges);
        assert(freeBySize_.size() == header_.freeRanges);
        // dumpFreeRanges();

        return firstPage;
    }

    uint32_t firstPage = header_.totalPages;
    uint32_t pagesPerSegment = SEGMENT_LENGTH >> store_.pageSizeShift_;
    int remainingPages = pagesPerSegment - (firstPage & (pagesPerSegment - 1));
    if (remainingPages < requestedPages)
    {
        // If the blob won't fit into the current segment, we'll
        // mark the remaining space as a free blob, and allocate
        // the blob in a fresh segment

        uint32_t firstRemainingPage = header_.totalPages;
        firstPage = firstRemainingPage + remainingPages;
        header_.totalPages = firstPage + requestedPages;

        freeBySize_.insert(
            (static_cast<uint64_t>(remainingPages) << 32) |
            firstRemainingPage);
        freeByStart_.insert(
            (static_cast<uint64_t>(firstRemainingPage) << 32) |
            (remainingPages << 1));
        ++header_.freeRanges;

        //LOGS << "  Allocated virgin " << requestedPages << " pages at "
        //    << firstPage << ", skipping " << remainingPages << " at "
        //    << firstRemainingPage << "\n";

        // dumpFreeRanges();

        assert(freeByStart_.size() == header_.freeRanges);
        assert(freeBySize_.size() == header_.freeRanges);
        return firstPage;
    }

    //LOGS << "  Allocated virgin " << requestedPages
    //    << " pages at " << firstPage << "\n";

    header_.totalPages = firstPage + requestedPages;
    assert(freeByStart_.size() == header_.freeRanges);
    assert(freeBySize_.size() == header_.freeRanges);
    return firstPage;
}

// TODO: Doesn't work, erase() invalidates iterators
void FreeStore::Transaction::performFreePages(uint32_t firstPage, uint32_t pages)
{
    assert(pages > 0);
    assert(pages <= SEGMENT_LENGTH >> store_.pageSizeShift_);

    // header_.freeRangeIndex = INVALID_FREE_RANGE_INDEX;

#ifndef NDEBUG
    // Guard against overlapping free

    auto itNext = freeByStart_.lower_bound(
        static_cast<uint64_t>(firstPage) << 32);
    if (itNext != freeByStart_.end())
    {
        uint32_t nextFirstPage = static_cast<uint32_t>(*itNext >> 32);
        if (nextFirstPage < firstPage + pages)
        {
            LOGS << "Attempting to free a range (" << firstPage << ":" << pages
                << ") that overlaps free range at " << nextFirstPage;
            assert(false);
        }
    }

    auto itPrev = freeByStart_.upper_bound(
        static_cast<uint64_t>(firstPage) << 32);
    if (itPrev != freeByStart_.begin())
    {
        --itPrev;
        uint32_t prevFirstPage = static_cast<uint32_t>(*itPrev >> 32);
        uint32_t prevSize = static_cast<uint32_t>(*itPrev) >> 1;
        if (prevFirstPage + prevSize > firstPage)
        {
            LOGS << "Attempting to free a range at " << firstPage
                << " that overlaps free range at " << prevFirstPage
                << ":" << prevSize;
            assert(false);
        }
    }

#endif
    // checkFreeTrees();
    LOGS << firstPage << ": Freeing " << pages << " pages\n";

    if (firstPage + pages == header_.totalPages)
    {
        // Blob is at end -> trim the file
        header_.totalPages -= pages;
        while (!freeByStart_.empty())
        {
            auto it = std::prev(freeByStart_.end());
            uint32_t first = static_cast<uint32_t>(*it >> 32);
            uint32_t size = static_cast<uint32_t>(*it) >> 1;
            if (first + size != header_.totalPages) break;
            header_.totalPages = first;
            uint64_t sizeKey =
                (static_cast<uint64_t>(size) << 32) | first;
            auto itSize = freeBySize_.lower_bound(sizeKey);
            assert(itSize != freeBySize_.end() && *itSize == sizeKey);
            freeBySize_.erase(itSize);
            freeByStart_.erase(it);
            --header_.freeRanges;

            LOGS << first << ": Trimmed " << size << " from end\n";
        }
        assert(freeByStart_.size() == header_.freeRanges);
        assert(freeBySize_.size() == header_.freeRanges);
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
            right = freeByStart_.erase(right);
            auto itSize = freeBySize_.lower_bound(
                (static_cast<uint64_t>(rightSize) << 32) | rightStart);
            if (itSize == freeBySize_.end() ||
                (*itSize >> 32) != rightSize ||
                static_cast<uint32_t>(*itSize) != rightStart)
            {
                assert(false);
            }
            freeBySize_.erase(itSize);
            --header_.freeRanges;

            LOGS << "Coalesced right free blob at " << rightStart << "\n";
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
            right = freeByStart_.erase(left);
            auto itSize = freeBySize_.lower_bound(
                (static_cast<uint64_t>(leftSize) << 32) | leftStart);
            if (itSize == freeBySize_.end() ||
                (*itSize >> 32) != leftSize ||
                static_cast<uint32_t>(*itSize) != leftStart)
            {
                assert(false);
            }
            freeBySize_.erase(itSize);
            --header_.freeRanges;

            LOGS << "Coalesced into left free blob at " << leftStart << "\n";
        }
    }
    freeByStart_.insert(right,
        (static_cast<uint64_t>(firstPage) << 32) |
        (pages << 1) | 1);
        // TODO: hint asserts
    freeBySize_.insert(
        (static_cast<uint64_t>(pages) << 32) | firstPage);
    ++header_.freeRanges;
    if (freeByStart_.size() != header_.freeRanges)
    {
        LOGS << "Free ranges per header: " << header_.freeRanges
             << " vs. free ranges in freeByStart_: " << freeByStart_.size() << "\n";
    }
    assert(freeByStart_.size() == header_.freeRanges);
    assert(freeBySize_.size() == header_.freeRanges);
}

/// @brief Allocates a new blob for the FRI and writes it.
///
void FreeStore::Transaction::writeFreeRangeIndex()
{
    if (freeByStart_.size() != freeBySize_.size() ||
        freeByStart_.size() != header_.freeRanges)
    {
        //LOGS << "Free by start: " << freeByStart_.size()
        //    << "\nFree by size:  " << freeBySize_.size()
        //    << "\nFree ranges:   " << header_.freeRanges;
    }
    assert(freeByStart_.size() == freeBySize_.size());
    assert(freeByStart_.size() == header_.freeRanges);

    if (header_.freeRanges == 0) [[unlikely]]
    {
        header_.freeRangeIndex = 0;
        return;
    }

    // After allocating the blob to hold the FRI, the number of
    // free ranges may differ: It may be one less (if a free range
    // of the exact size has been found), or one more (a blob has
    // been appended, but the leftover tail of a 1-GB segment has
    // been added as a new free range). The free-range count only
    // stays the same if we're allocating a portion of an existing
    // free range; therefore, we must request enough space to
    // store one more entry

    // +1 for the blob's header (8 bytes) and for one extra entry
    // (header_.freeRanges may change after the call to allocPages)
    uint32_t slotCount = header_.freeRanges + 2;
    uint32_t indexSize = slotCount * sizeof(uint64_t);
    uint32_t indexSizeInPages = store_.pagesForBytes(indexSize);
    uint32_t indexPage = allocPages(indexSizeInPages);

    std::unique_ptr<uint64_t[]> index(new uint64_t[slotCount]);
    uint64_t* p = index.get();
    uint64_t* pEnd = p + slotCount;
    *p++ = (slotCount * 8) - 4;
    uint64_t prevEntry = 0;
    for (auto entry : freeByStart_)
    {
        *p++ = entry;
        assert(entry > prevEntry);
        prevEntry = entry;
    }
    assert(p == index.get() + header_.freeRanges + 1);

    // put zeroes into leftover slots
    if (p < pEnd)   [[likely]]
    {
        *p++ = 0;
        if (p < pEnd)   [[unlikely]]
        {
            *p = 0;
        }
    }

    store_.file_.writeAllAt(store_.offsetOfPage(indexPage),
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
    LOGS << header_.totalPages << " total pages\n";
    LOGS << "Free ratio: " << (static_cast<double>(startSize) / header_.totalPages) << "\n";
    assert(startCount == sizeCount);
    assert(startSize == sizeSize);
}

void FreeStore::Transaction::beginCreateStore()
{
//    memset(&header_, 0, sizeof(header_));

    std::random_device rd;
    std::mt19937_64 gen(rd());  // 64-bit Mersenne Twister engine
    // Uniform distribution over the whole uint64_t range
    std::uniform_int_distribution<uint64_t> dist(
        std::numeric_limits<uint64_t>::min(),
        std::numeric_limits<uint64_t>::max()
    );
    header_.commitId = dist(gen);
    header_.pageSizeShift = 12;     // 4KB blocks
    header_.totalPages = 1;
}

void FreeStore::Transaction::endCreateStore()
{

}

/// @brief Reads the FRI and frees the blob in which it is stored.
///
void FreeStore::Transaction::readFreeRangeIndex()
{
    uint32_t count = header_.freeRanges;
    if (count == 0) [[unlikely]]
    {
        return;
    }

    std::unique_ptr<uint64_t[]> ranges(new uint64_t[count]);
    store_.file_.readAllAt(
        store_.offsetOfPage(header_.freeRangeIndex),
        ranges.get(), (count + 1) * sizeof(uint64_t));

    uint32_t prevStart = 0;
    for (int i=0; i<count; i++)
    {
        uint64_t range = ranges[i];
        freeByStart_.insert(freeByStart_.end(), range);
        uint32_t first = static_cast<uint32_t>(range >> 32);
        uint32_t size = static_cast<uint32_t>(range) >> 1;
        ranges[i] = (static_cast<uint64_t>(size) << 32) | first;
        assert(first > prevStart);
        prevStart = first;
    }

    std::sort(ranges.get() + 1, ranges.get() + count + 1);
    for (int i=1; i <= count; i++)
    {
        uint64_t range = ranges[i];
        freeBySize_.insert(freeBySize_.end(), range);
    }

    // Free the FRI's blob
    // (Its free range doesn't get added to the trees yet, to prevent
    // it from being reallocated within the first commit cycle, before
    // the INVALID_FREE_RANGE_INDEX has been committed)

    uint32_t indexBlobSize = ranges[0] + 4;
    freePages(header_.freeRangeIndex, store_.pagesForBytes(indexBlobSize));
    header_.freeRangeIndex = INVALID_FREE_RANGE_INDEX;

    assert(freeByStart_.size() == count);
    assert(freeBySize_.size() == count);
}

uint32_t FreeStore::Transaction::addBlob(std::span<const byte> data)
{
    assert(data.size() <= SEGMENT_LENGTH);
    uint32_t firstPage = allocPages(store_.pagesForBytes(data.size()));
    uint64_t ofs = static_cast<uint64_t>(firstPage) << store_.pageSizeShift_;
    store_.file_.writeAllAt(ofs, data);
    return firstPage;
}

} // namespace clarisma


