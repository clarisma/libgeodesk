// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/libero/FreeStore_Transaction.h>
#include <clarisma/util/Crc32.h>
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
    if (isFinal)
    {
        if (header_.freeRangeIndex == INVALID_FREE_RANGE_INDEX)
        {
            writeFreeRangeIndex();
        }
    }

    header_.commitId++;
    Crc32C crc;
    crc.update(&header_, CHECKSUMMED_HEADER_SIZE);
    header_.checksum = crc.get();
    LOGS << "Calculated header checksum: " << header_.checksum;

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

    header_.freeRangeIndex = INVALID_FREE_RANGE_INDEX;

    // checkFreeTrees();
    LOGS << "Allocating " << requestedPages << " pages\n";

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
            LOGS << "  Found perfect fit of " << requestedPages
                << " pages at " << firstPage;

            // Perfect fit
            freeByStart_.erase(it);
            --header_.freeRanges;

            dumpFreeRanges();

            assert(freeByStart_.size() == header_.freeRanges);
            assert(freeBySize_.size() == header_.freeRanges);
            return firstPage;
        }

        // we need to give back the remaining chunk

        LOGS << "  Allocated " << requestedPages
            << " pages at " << firstPage << ", giving back "
            << (freePages - requestedPages) << " at " <<
                (firstPage + requestedPages) << "\n";

        bool garbageFlag = static_cast<bool>(*it & 1);
        uint32_t leftoverSize = freePages - requestedPages;
        uint32_t leftoverStart = firstPage + requestedPages;
        //  auto hint = std::next(it);
        it = freeByStart_.erase(it);
        freeByStart_.insert(it,
            (static_cast<uint64_t>(leftoverStart) << 32) |
            (leftoverSize << 1) |
            garbageFlag);
        // TODO: hint asserts

        freeBySize_.insert((static_cast<uint64_t>(leftoverSize) << 32) | leftoverStart);

        assert(freeByStart_.size() == header_.freeRanges);
        assert(freeBySize_.size() == header_.freeRanges);
        dumpFreeRanges();

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

        LOGS << "  Allocated virgin " << requestedPages << " pages at "
            << firstPage << ", skipping " << remainingPages << " at "
            << firstRemainingPage;

        dumpFreeRanges();

        assert(freeByStart_.size() == header_.freeRanges);
        assert(freeBySize_.size() == header_.freeRanges);
        return firstPage;
    }

    LOGS << "  Allocated virgin " << requestedPages
        << " pages at " << firstPage;

    header_.totalPages = firstPage + requestedPages;
    assert(freeByStart_.size() == header_.freeRanges);
    assert(freeBySize_.size() == header_.freeRanges);
    return firstPage;
}

// TODO: Doesn't work, erase() invalidates iterators
void FreeStore::Transaction::freePages(uint32_t firstPage, uint32_t pages)
{
    assert(pages > 0);
    assert(pages <= SEGMENT_LENGTH >> store_.pageSizeShift_);

    header_.freeRangeIndex = INVALID_FREE_RANGE_INDEX;

    // checkFreeTrees();
    // LOGS << firstPage << ": Freeing " << pages << " pages\n";

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

            // LOGS << first << ": Trimmed " << size << " from end";
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
        }
    }
    freeByStart_.insert(right,
        (static_cast<uint64_t>(firstPage) << 32) |
        (pages << 1) | 1);
        // TODO: hint asserts
    freeBySize_.insert(
        (static_cast<uint64_t>(pages) << 32) | firstPage);
    ++header_.freeRanges;
    assert(freeByStart_.size() == header_.freeRanges);
    assert(freeBySize_.size() == header_.freeRanges);
}

void FreeStore::Transaction::writeFreeRangeIndex()
{
    if (freeByStart_.size() != freeBySize_.size() ||
        freeByStart_.size() != header_.freeRanges)
    {
        LOGS << "Free by start: " << freeByStart_.size()
            << "\nFree by size:  " << freeBySize_.size()
            << "\nFree ranges:   " << header_.freeRanges;
    }
    assert(freeByStart_.size() == freeBySize_.size());
    assert(freeByStart_.size() == header_.freeRanges);

    if (header_.freeRanges == 0) [[unlikely]]
    {
        header_.freeRangeIndex = 0;
        return;
    }
    uint32_t indexSize = (header_.freeRanges + 1) * sizeof(uint64_t);
    uint32_t indexSizeInPages = store_.pagesForBytes(indexSize);
    auto it = freeBySize_.lower_bound(
        static_cast<uint64_t>(indexSizeInPages) << 32);
    uint32_t indexPage;
    if (it != freeBySize_.end()) [[likely]]
    {
        indexPage = static_cast<uint32_t>(*it);
    }
    else
    {
        indexPage = header_.totalPages;
        header_.totalPages += indexSizeInPages;
        // TODO: clarify if the FRI is allowed to straddle
        //  segment boundaries (it is not used by consumers
        //  that may require segment-based mapping)
    }
    std::unique_ptr<uint64_t[]> index(new uint64_t[header_.freeRanges + 1]);
    uint64_t* p = index.get();
    *p++ = (header_.freeRanges * 8) + 4;
    for (auto entry : freeByStart_)
    {
        *p++ = entry;
    }
    assert(p - index.get() == header_.freeRanges + 1);

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

void FreeStore::Transaction::readFreeRangeIndex()
{
    uint32_t count = header_.freeRanges;
    if (count == 0) return;
    std::unique_ptr<uint64_t[]> ranges(new uint64_t[count]);
    store_.file_.readAllAt(
        (header_.freeRangeIndex << store_.pageSizeShift_) + 8,
        ranges.get(), count * sizeof(uint64_t));
    for (int i=0; i<count; i++)
    {
        uint64_t range = ranges[i];
        freeByStart_.insert(freeByStart_.end(), range);
        uint32_t first = static_cast<uint32_t>(range >> 32);
        uint32_t size = static_cast<uint32_t>(range) >> 1;
        ranges[i] = (static_cast<uint64_t>(size) << 32) | first;
    }
    std::sort(ranges.get(), ranges.get() + count);
    for (int i=0; i<count; i++)
    {
        uint64_t range = ranges[i];
        freeBySize_.insert(freeBySize_.end(), range);
    }
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


