// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/store/BlobStore.h>
#include <clarisma/util/Bits.h>
#include <clarisma/util/ShortVarString.h>

namespace clarisma {

// TODO: must move JournalFile to store
//  because inside a CreateTransaction it is initialzied
//  before the Store itself is valid (it is a member of CreateTransaction,
//  therefore initialzied after base class Transaction)

DateTime BlobStore::getLocalCreationTimestamp() const
{
	return getRoot()->creationTimestamp;
}

uint64_t BlobStore::getTrueSize() const 
{
    return static_cast<uint64_t>(getRoot()->totalPageCount) << pageSizeShift_;
}

// TODO: When does pageSize get set when creating a new BlobStore?
//  This affects the true size of the store
//  It must be set before Transaction::begin() is called
void BlobStore::initialize(bool create)
{
    Header* root = getRoot();
    if (root->magic != MAGIC)
    {
        if(create && root->magic == 0)
        {
            root->totalPageCount = 1;
        }
        else
        {
            error("Not a BlobStore file");
        }
    }
}

uint32_t BlobStore::pagesForPayloadSize(uint32_t payloadSize) const
{
    assert(payloadSize <= SEGMENT_LENGTH - BLOB_HEADER_SIZE);
    return (payloadSize + (1 << pageSizeShift_) + BLOB_HEADER_SIZE - 1) >> pageSizeShift_;
}


/**
 * Allocates a blob of a given size. If possible, the smallest existing 
 * free blob that can accommodate the requested number of bytes will 
 * be reused; otherwise, a new blob will be appended to the store file.
 *
 * TODO: guard against exceeding maximum file size
 *
 * @param size   the size of the blob, not including its 4-byte header
 * @return       the first page of the blob
 */
BlobStore::PageNum BlobStore::Transaction::alloc(uint32_t payloadSize)
{
    assert (payloadSize <= SEGMENT_LENGTH - BLOB_HEADER_SIZE);
    // int precedingBlobFreeFlag = 0;
    uint32_t requiredPages = store()->pagesForPayloadSize(payloadSize);
    HeaderBlock* rootBlock = getRootBlock();
    uint32_t trunkRanges = rootBlock->trunkFreeTableRanges;
    if (trunkRanges != 0)
    {
        // If there are free blobs, check if there is one large enough
        // to accommodate the requested size

        // The first slot in the trunk FT worth checking
        uint32_t trunkSlot = (requiredPages - 1) / 512;
        uint32_t trunkSlotEnd = (trunkSlot & 0x1f0) + 16;

        // The first slot in the leaf FT to check (for subsequent
        // leaf FTs, we need to start at 0
        uint32_t leafSlot = (requiredPages - 1) % 512;
        // int trunkOfs = TRUNK_FREE_TABLE_OFS + trunkSlot * 4;
        // int trunkEnd = (trunkOfs & 0xffff'ffc0) + 64;

        // We don't care about ranges that cover page counts that are less
        // than the number of pages we require, so shift off those bits
        trunkRanges >>= trunkSlot / 16;

        for (;;)
        {
            if ((trunkRanges & 1) == 0)
            {
                // There are no free blobs in the target range, so let's
                // check free blobs in the next-larger range; if there
                // aren't any, we quit

                if (trunkRanges == 0) break;

                // If a range does not contain any free blobs, no need
                // to check each of its 16 entries individually. The number
                // of zero bits tells us how many ranges we can skip

                int rangesToSkip = Bits::countTrailingZerosInNonZero(trunkRanges);
                trunkSlotEnd += rangesToSkip * 16;
                trunkSlot = trunkSlotEnd - 16;
                // trunkEnd += rangesToSkip * 64;
                // trunkOfs = trunkEnd - 64;

                // for any range other than the first, we check all leaf slots

                leafSlot = 0;
            }
            // assert(trunkOfs < TRUNK_FREE_TABLE_OFS + FREE_TABLE_LEN);

            for (; trunkSlot < trunkSlotEnd; trunkSlot++)
            {
                PageNum leafTableBlob = rootBlock->trunkFreeTable[trunkSlot];
                if (leafTableBlob == 0) continue;

                // There are one or more free blobs within the
                // 512-page size range indicated by trunkOfs

                Blob* leafBlock = getBlobBlock(leafTableBlob);
                int leafRanges = leafBlock->leafFreeTableRanges;
                uint32_t leafSlotEnd = (leafSlot & 0x1f0) + 16;
                assert(leafBlock->isFree);

                leafRanges >>= leafSlot / 16;

                for (; ; )
                {
                    if ((leafRanges & 1) == 0)
                    {
                        if (leafRanges == 0) break;
                        int rangesToSkip = Bits::countTrailingZerosInNonZero(leafRanges);
                        leafSlotEnd += rangesToSkip * 16;
                        leafSlot = leafSlotEnd - 16;
                    }
                    for (; leafSlot < leafSlotEnd; leafSlot++)
                    {
                        PageNum freeBlob = leafBlock->leafFreeTable[leafSlot];
                        if (freeBlob == 0) continue;

                        // Found a free blob of sufficient size

                        // TODO: Do not re-allocate blobs from freedBlobs_

                        uint32_t freePages = trunkSlot * 512 + leafSlot + 1;
                        if (freeBlob == leafTableBlob)
                        {
                            // If the free blob is the same blob that holds
                            // the leaf free-table, check if there is another
                            // free blob of the same size

                            // log.debug("    Candidate free blob {} holds leaf FT", freeBlob);

                            PageNum nextFreeBlob = leafBlock->nextFreeBlob;
                            if (nextFreeBlob != 0)
                            {
                                // If so, we'll use that blob instead

                                // log.debug("    Using next free blob at {}", nextFreeBlob);
                                freeBlob = nextFreeBlob;
                            }
                        }

                        // TODO!!!!!
                        // TODO: bug: we need to relocate ft after we
                        //  add the remaining part
                        //  free blob is last of size, remaining is in same leaf FT
                        //  won't move the FT, FT ends up in allocated portion
                        //  OR: remove entire blob first,then add remaining?
                        //  Solution: reverse sequence: remove whole blob first,
                        //  then add back the remaining part

                        Blob* freeBlock = getBlobBlock(freeBlob);
                        assert(freeBlock->isFree);
                        uint32_t freeBlobPayloadSize = freeBlock->payloadSize;
                        assert((freeBlobPayloadSize + BLOB_HEADER_SIZE) >> 
                            store()->pageSizeShift_ == freePages);
                        assert (freePages >= requiredPages);
                        removeFreeBlob(freeBlock);

                        if (freeBlob == leafTableBlob)
                        {
                            // We need to move the freetable to another free blob
                            // (If it is no longer needed, this is a no-op;
                            // removeFreeBlob has already set the trunk slot to 0)
                            // TODO: consolidate with removeFreeBlob?
                            //  We only separate this step because in freeBlob
                            //  we are potentially removing preceding/following
                            //  blob of same size range, which means we'd have
                            //  to move FT twice

                            PageNum newLeafBlob = relocateFreeTable(freeBlob, freePages);
                            if (newLeafBlob != 0)
                            {
                                // log.debug("    Moved leaf FT to {}", newLeafBlob);
                                assert (rootBlock->trunkFreeTable[trunkSlot] == newLeafBlob);
                            }
                            else
                            {
                                // log.debug("    Leaf FT no longer needed");
                                assert (rootBlock->trunkFreeTable[trunkSlot] == 0);
                            }
                        }

                        if (freePages > requiredPages)
                        {
                            // If the free blob is larger than needed, mark the
                            // remainder as free and add it to its respective free list;
                            // we always do this before we remove the reused blob, since
                            // we may needlessly remove and reallocate the free table
                            // if the reused is the last blob in the table, but the
                            // remainder is in the same 512-page range

                            // We won't need to touch the preceding-free flag of the
                            // successor blob, since it is already set

                            addFreeBlob(freeBlob + requiredPages, freePages - requiredPages, 0);
                        }
                        Blob* nextBlock = getBlobBlock(freeBlob + freePages);
                        nextBlock->precedingFreeBlobPages = freePages - requiredPages;

                        freeBlock->isFree = false;
                        freeBlock->payloadSize = payloadSize;
                        // debugCheckRootFT();
                        return freeBlob;
                    }
                    leafRanges >>= 1;
                    leafSlotEnd += 16;
                }
                leafSlot = 0;
            }

            // Check the next range

            trunkRanges >>= 1;
            trunkSlotEnd += 16;
        }
    }

    // If we weren't able to find a suitable free blob,
    // we'll grow the store

    uint32_t totalPages = rootBlock->totalPageCount;
    uint32_t pagesPerSegment = SEGMENT_LENGTH >> store()->pageSizeShift_;
    int remainingPages = pagesPerSegment - (totalPages & (pagesPerSegment - 1));
    uint32_t precedingFreePages = 0;
    if (remainingPages < requiredPages)
    {
        // If the blob won't fit into the current segment, we'll
        // mark the remaining space as a free blob, and allocate
        // the blob in a fresh segment

        addFreeBlob(totalPages, remainingPages, 0);
        totalPages += remainingPages;

        // In this case, we'll need to set the preceding-free flag of the
        // allocated blob

        precedingFreePages = remainingPages;
    }
    rootBlock->totalPageCount = totalPages + requiredPages;

    // TODO: no need to journal the blob's header block if it is in
    //  virgin space
    //  But: need to mark the segment as dirty, so it can be forced
    Blob* newBlock = getBlobBlock(totalPages);
    newBlock->precedingFreeBlobPages = precedingFreePages;
    newBlock->payloadSize = payloadSize;
    newBlock->isFree = false;
    // debugCheckRootFT();
    return totalPages;
}

/// Removes a free blob from its freetable. If this blob is the last 
/// free blob in a given size range, removes the leaf freetable from 
/// the trunk freetable. If this free blob contains the leaf freetable, 
/// and this freetable is still needed, it is the responsibility of 
/// the caller to copy it to another free blob in the same size range.
///
/// This method does not affect the precedingFreeBlobPages field of the 
/// successor blob; it is the responsibility of the caller to clear 
/// the flag, if necessary.
///
/// @param freeBlock The block representing the free blob.
///
void BlobStore::Transaction::removeFreeBlob(Blob* freeBlock)
{
    PageNum prevBlob = freeBlock->prevFreeBlob;
    PageNum nextBlob = freeBlock->nextFreeBlob;

    // Unlink the blob from its siblings

    if (nextBlob != 0)
    {
        Blob* nextBlock = getBlobBlock(nextBlob);
        nextBlock->prevFreeBlob = prevBlob;
    }
    if (prevBlob != 0)
    {
        Blob* prevBlock = getBlobBlock(prevBlob);
        prevBlock->nextFreeBlob = nextBlob;
        return;
    }

    // Determine the free blob that holds the leaf freetable
    // for free blobs of this size

    uint32_t payloadSize = freeBlock->payloadSize;
    int pages = (payloadSize + BLOB_HEADER_SIZE) >> store()->pageSizeShift_;
    int trunkSlot = (pages - 1) / 512;
    int leafSlot = (pages - 1) % 512;

    // log.debug("     Removing blob with {} pages", pages);

    HeaderBlock* rootBlock = getRootBlock();
    PageNum leafBlob = rootBlock->trunkFreeTable[trunkSlot];

    // If the leaf FT has already been dropped, there's nothing
    // left to do (TODO: this feels messy)
    // (If we don't check, we risk clobbering the root FT)
    // TODO: leafBlob should never be 0!

    assert (leafBlob != 0);

    // if(leafBlob == 0) return;

    // Set the next free blob as the first blob of its size

    Blob* leafBlock = getBlobBlock(leafBlob);
    leafBlock->leafFreeTable[leafSlot] = nextBlob;
    if (nextBlob != 0) return;

    // Check if there are any other free blobs in the same size range

    uint32_t leafRange = leafSlot / 16;
    assert (leafRange >= 0 && leafRange < 32);

    uint32_t startLeafSlot = leafRange * 16;
    uint32_t endLeafSlot = startLeafSlot + 16;
    for (uint32_t slot = startLeafSlot; slot<endLeafSlot; slot++)
    {
        if (leafBlock->leafFreeTable[slot] != 0) return;
    }

    // The range has no free blobs, clear its range bit

    leafBlock->leafFreeTableRanges &= ~(1 << leafRange); 
    if (leafBlock->leafFreeTableRanges != 0) return;

    // No ranges are in use, which means the leaf free table is
    // no longer required

    rootBlock->trunkFreeTable[trunkSlot] = 0;

    // Check if there are any other leaf tables in the same size range

    uint32_t trunkRange = trunkSlot / 16;
    assert (trunkRange >= 0 && trunkRange < 32);
    uint32_t startTrunkSlot = trunkRange * 16;
    uint32_t endTrunkSlot = startTrunkSlot + 16;
    for (uint32_t slot = startTrunkSlot; slot < endTrunkSlot; slot++)
    {
        if (rootBlock->trunkFreeTable[slot] != 0) return;
    }

    // The trunk range has no leaf tables, clear its range bit
    rootBlock->trunkFreeTableRanges &= ~(1 << trunkRange);
}


/// Adds a blob to the freetable, and sets its size, header flags, and trailer.
///
/// This method does not affect the precedingFreeBlobPages field of the 
/// successor blob; it is the responsibility of the caller to set this, 
/// if necessary.
///
/// @param firstPage the first page of the blob
/// @param pages     the number of pages of this blob
/// @param precedingFreePages the number of preceding free pages
///
void BlobStore::Transaction::addFreeBlob(PageNum firstPage, uint32_t pages, uint32_t precedingFreePages)
{
    Blob* block = getBlobBlock(firstPage);
    block->precedingFreeBlobPages = precedingFreePages;
    block->payloadSize = (pages << store()->pageSizeShift_) - BLOB_HEADER_SIZE;
    block->isFree = true;
    block->prevFreeBlob = 0;
    HeaderBlock* rootBlock = getRootBlock();
    uint32_t trunkSlot = (pages - 1) / 512;
    PageNum leafBlob = rootBlock->trunkFreeTable[trunkSlot];
    Blob* leafBlock;
    if (leafBlob == 0)
    {
        // If no local free table exists for the size range of this blob,
        // this blob becomes the local free table

        block->leafFreeTableRanges = 0;
        memset(block->leafFreeTable, 0, sizeof(block->leafFreeTable));
        rootBlock->trunkFreeTableRanges |= 1 << (trunkSlot / 16);
        rootBlock->trunkFreeTable[trunkSlot] = firstPage;
        leafBlock = block;

        // Log.debug("  Free blob %d: Created leaf FT for %d pages", firstPage, pages);
    }
    else
    {
        leafBlock = getBlobBlock(leafBlob);
    }

    // Determine the slot in the leaf freetable where this
    // free blob will be placed

    uint32_t leafSlot = (pages - 1) % 512;
    PageNum nextBlob = leafBlock->leafFreeTable[leafSlot];
    if (nextBlob != 0)
    {
        // If a free blob of the same size exists already,
        // chain this blob as a sibling

        Blob* nextBlock = getBlobBlock(nextBlob);
        nextBlock->prevFreeBlob = firstPage;
    }
    block->nextFreeBlob = nextBlob;

    // Enter this free blob into the size-specific slot
    // in the leaf freetable, and set the range bit

    leafBlock->leafFreeTable[leafSlot] = firstPage;
    leafBlock->leafFreeTableRanges |= 1 << (leafSlot / 16);
}


/**
 * Deallocates a blob. Any adjacent free blobs are coalesced, 
 * provided that they are located in the same 1-GB segment.
 *
 * @param firstPage the first page of the blob
 */
void BlobStore::Transaction::free(PageNum firstPage)
{
    HeaderBlock* rootBlock = getRootBlock();
    Blob* block = getBlobBlock(firstPage);
    
    assert(!block->isFree);
    /*
    if (freeFlag != 0)
    {
        throw new StoreException(
            "Attempt to free blob that is already marked as free", path());
    }
    */

    uint32_t totalPages = rootBlock->totalPageCount;

    uint32_t pages = store()->pagesForPayloadSize(block->payloadSize);
    PageNum prevBlob = 0;
    PageNum nextBlob = firstPage + pages;
    uint32_t prevPages = block->precedingFreeBlobPages;
    uint32_t nextPages = 0;

    if (prevPages && !isFirstPageOfSegment(firstPage))
    {
        // If there is another free blob preceding this one,
        // and it is in the same segment, coalesce it

        prevBlob = firstPage - prevPages;
        Blob* prevBlock = getBlobBlock(prevBlob);

        // TODO: check:    
        // The preceding free blob could itself have a preceding free blob
        // (not coalesced because it is located in different segment),
        // so we preserve the preceding_free_flag

        removeFreeBlob(prevBlock);
        block = prevBlock;
        // log.debug("    Coalescing preceding blob at {} ({} pages})", prevBlob, prevPages);
    }

    if (nextBlob < totalPages && !isFirstPageOfSegment(nextBlob))
    {
        // There is another blob following this one,
        // and it is in the same segment

        Blob* nextBlock = getBlobBlock(nextBlob);
        if (nextBlock->isFree)
        {
            // The next blob is free, coalesce it

            nextPages = store()->pagesForPayloadSize(nextBlock->payloadSize);
            removeFreeBlob(nextBlock);

            // log.debug("    Coalescing following blob at {} ({} pages})", nextBlob, nextPages);
        }
    }

    if (prevPages != 0)
    {
        uint32_t trunkSlot = (prevPages - 1) / 512;
        PageNum prevFreeTableBlob = rootBlock->trunkFreeTable[trunkSlot];
        if (prevFreeTableBlob == prevBlob)
        {
            relocateFreeTable(prevFreeTableBlob, prevPages);
        }
    }
    if (nextPages != 0)
    {
        uint32_t trunkSlot = (nextPages - 1) / 512;
        PageNum nextFreeTableBlob = rootBlock->trunkFreeTable[trunkSlot];
        if (nextFreeTableBlob == nextBlob)
        {
            relocateFreeTable(nextFreeTableBlob, nextPages);
        }
    }

    pages += prevPages + nextPages;
    firstPage -= prevPages;

    /*
    if(pages == 262144)
    {
        Log.debug("Freeing 1-GB blob (First page = %d @ %X)...",
            firstPage, (long)firstPage << pageSizeShift);
    }
     */

    if (firstPage + pages == totalPages)
    {
        // If the free blob is located at the end of the file, reduce
        // the total page count (effectively truncating the store)

        totalPages = firstPage;
        while (block->precedingFreeBlobPages)
        {
            // If the preceding blob is free, that means it
            // resides in the preceding 1-GB segment (since we cannot
            // coalesce across segment boundaries). Remove this blob
            // from its freetable and trim it off. If this blob
            // occupies an entire segment, and its preceding blob is
            // free as well, keep trimming

            // Log.debug("Trimming across segment boundary...");

            prevPages = block->precedingFreeBlobPages;
            totalPages -= prevPages;
            prevBlob = totalPages;
            block = getBlobBlock(prevBlob);
            removeFreeBlob(block);

            // Move freetable, if necessary

            uint32_t trunkSlot = (prevPages - 1) / 512;
            PageNum prevFreeTableBlob = rootBlock->trunkFreeTable[trunkSlot];
            if (prevFreeTableBlob == prevBlob)
            {
                // Log.debug("Relocating free table for %d pages", prevPages);
                relocateFreeTable(prevBlob, prevPages);
            }

            if (!isFirstPageOfSegment(totalPages)) break;
        }
        rootBlock->totalPageCount = totalPages;

        // Log.debug("Truncated store to %d pages.", totalPages);
    }
    else
    {
        // Blob is not at end of file, add it to the free table

        addFreeBlob(firstPage, pages, block->precedingFreeBlobPages);
        Blob* nextBlock = getBlobBlock(firstPage + pages);
        nextBlock->precedingFreeBlobPages = pages;
    }

    // Track this freed blob so we don't re-allocate it within
    // the same transaction (If we were to re-allocate freed blobs,
    // we would have to journal their entire contents in order to
    // ensure that the transaction can be fully rolled back in case
    // of failure)

    freedBlobs_.insert({ firstPage, pages });
}

/// Copies a blob's free table to another free blob. The original blob's 
/// free table and the free-range bits must be valid, all other data is 
/// allowed to have been modified at this point.
///
/// @param page        the first page of the original blob
/// @param sizeInPages the blob's size in pages
/// @return the page of the blob to which the free table has been assigned, 
///         or 0 if the table has not been relocated.
///
BlobStore::PageNum BlobStore::Transaction::relocateFreeTable(PageNum page, int sizeInPages)
{
    Blob* block = getBlobBlock(page);
    uint32_t ranges = block->leafFreeTableRanges;
    // Make a copy of ranges, because we will modify it during search
    uint32_t originalRanges = ranges;
    uint32_t slot = 0;
    while (ranges != 0)
    {
        if ((ranges & 1) != 0)
        {
            uint32_t endSlot = slot + 16;
            for (; slot < endSlot; slot++)
            {
                PageNum otherPage = block->leafFreeTable[slot];
                if (otherPage != 0 && otherPage != page)
                {
                    Blob* otherBlock = getBlobBlock(otherPage);
                    assert(otherBlock->isFree);

                    memcpy(otherBlock->leafFreeTable, block->leafFreeTable, sizeof(block->leafFreeTable));
                    otherBlock->leafFreeTableRanges = originalRanges;
                    // don't use `ranges`; search consumes the bits
                    HeaderBlock* rootBlock = getRootBlock();
                    uint32_t trunkSlot = (sizeInPages - 1) / 512;
                    rootBlock->trunkFreeTable[trunkSlot] = otherPage;

                    // log.debug("      Moved free table from {} to {}", page, otherPage);
                    return otherPage;
                }
            }
            slot = endSlot;
            ranges >>= 1;
        }
        else
        {
            int rangesToSkip = Bits::countTrailingZerosInNonZero(ranges);
            ranges >>= rangesToSkip;
            slot += rangesToSkip * 16;
        }
    }
    return 0;
}


BlobStore::PageNum BlobStore::Transaction::addBlob(const ByteSpan data)
{
    PageNum firstPage = alloc(data.size());
    Blob* blob = getBlobBlock(firstPage);
    size_t firstPayloadSize = JournaledBlock::SIZE - 8;
    if (data.size() <= firstPayloadSize)
    {
        // All the data fits into the first block
        memcpy(blob->payload, data.data(), data.size());
    }
    else
    {
        // We only journal the first block (because it may contain freelist
        // data that we would otherwise overwrite in an unsafe way), but the
        // rest of the payload we write directly into the memory-mapped file
        memcpy(blob->payload, data.data(),firstPayloadSize);
        byte* unjournaledPayload = store()->translatePage(firstPage) + JournaledBlock::SIZE;
        memcpy(unjournaledPayload, data.data() + firstPayloadSize, data.size() - firstPayloadSize);
    }
    return firstPage;
}


void BlobStore::Transaction::commit()
{
    Store::Transaction::commit();
    // TODO: Deallocate pages of freed blobs ("punch holes")
    for (const auto& it : freedBlobs_)
    {
        PageNum firstPage = it.first;
        uint32_t pages = it.second;

        uint64_t ofs = store()->offsetOf(firstPage);
        uint64_t size = store()->offsetOf(pages);

        ofs += JournaledBlock::SIZE;
        size -= JournaledBlock::SIZE;
        store()->deallocate(ofs, size);

        // TODO: punch hole (but respect filesystem block sizes)
        // Do not deallocate the first 4KB block, as it contains
        // metadata
    }
}

void BlobStore::setMetadataSize(Header* header, size_t size)
{
    header->metadataSize = size;
    int pageSizeShift = header->pageSize + 12;
    size_t pageSize = 1 << pageSizeShift;
    header->totalPageCount = (size + pageSize - 1) >> pageSizeShift;
}

void BlobStore::Transaction::setup()
{
    HeaderBlock* header = getRootBlock();
    header->magic = MAGIC;
    header->versionHigh = 2;
    header->versionLow = 0;
    header->creationTimestamp = DateTime::now();
    header->pageSize = 0; // TODO: default
    header->totalPageCount = 1;
    header->metadataSize = 4096;        // TODO
    header->trunkFreeTableRanges = 0;
    memset(header->trunkFreeTable, 0, sizeof(header->trunkFreeTable));
}

std::map<std::string_view,std::string_view> BlobStore::properties() const
{
    std::map<std::string_view,std::string_view> properties;
    uint32_t ptr = getRoot()->propertiesPointer;
    if(ptr)
    {
        DataPtr p(mainMapping() + ptr);
        int count = p.getUnsignedShort();
        p += 2;
        for(int i=0; i<count; i++)
        {
            const ShortVarString* name = reinterpret_cast<const ShortVarString*>(p.ptr());
            p += name->totalSize();
            const ShortVarString* value = reinterpret_cast<const ShortVarString*>(p.ptr());
            p += value->totalSize();
            properties[name->toStringView()] = value->toStringView();
        }
    }
    return properties;
}


// =============== BTRee Functions ==================

/// @brief Finds the first entry whose key is >= x in the B-tree.
/// @param rootPage  page ID of the root node
/// @param x         search key
/// @return pointer to the matching Entry, or nullptr if none found
///
BlobStore::BTreeNode::Entry* BlobStore::Transaction::lowerBound(PageNum rootPage, uint32_t x) noexcept
{
    uint32_t page = rootPage;
    while (page)
    {
        BTreeNode* node = getNode(page);

        // Binary search in entries[0..count)
        uint32_t lo = 0;
        uint32_t hi = node->count;
        while (lo < hi)
        {
            uint32_t mid = (lo + hi) >> 1;
            if (node->entries[mid].key < x)
            {
                lo = mid + 1;
            }
            else
            {
                hi = mid;
            }
        }

        // If this is a leaf, either return the entry or nullptr
        if (node->leftmostChild == 0)
        {
            return (lo < node->count) ? &node->entries[lo] : nullptr;
        }

        // Otherwise descend: lo==0 ⇒ leftmostChild; else ⇒ entries[lo-1].valueOrChild
        page = (lo == 0) ?
            node->leftmostChild :
            node->entries[lo - 1].valueOrChild;
    }
    return nullptr;
}

BlobStore::SplitResult BlobStore::Transaction::insert(
    PageNum page, uint32_t key, uint32_t value)
{
    BTreeNode* node = getNode(page);
    SplitResult result{0, 0};

    // 1) Find insertion index via lower_bound
    uint32_t lo = 0, hi = node->count;
    while (lo < hi)
    {
        uint32_t mid = (lo + hi) >> 1;
        if (node->entries[mid].key < key)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }

    if (node->leftmostChild == 0)
    {
        // --- Leaf insertion ---
        // Shift entries [lo..count) right by one
        if (lo < node->count)
        {
            std::memmove(&node->entries[lo+1], &node->entries[lo],
                (node->count - lo) * sizeof(BTreeNode::Entry));
        }
        node->entries[lo].key = key;
        node->entries[lo].valueOrChild = value;
        node->count++;

        // No split needed
        if (node->count <= BTreeNode::MAX_ENTRIES)
        {
            return result;
        }

        // --- Split leaf ---
        uint32_t mid = node->count >> 1;             // e.g. 256
        PageNum newPage = allocNode();
        BTreeNode* sibling = getNode(newPage);
        // Leaf: sibling is also a leaf => leftmostChild = 0
        sibling->leftmostChild = 0;

        // Move entries [mid..count) into sibling
        uint32_t moveCount = node->count - mid;
        std::memcpy(&sibling->entries[0], &node->entries[mid],
            moveCount * sizeof(BTreeNode::Entry));
        sibling->count = moveCount;

        // Shrink the original
        node->count = mid;

        // Promote the first key of the sibling
        result.promoteKey  = sibling->entries[0].key;
        result.newPage     = newPage;
        return result;
    }

    // --- Internal node: descend ---
    uint32_t childPage = (lo == 0) ?
        node->leftmostChild : node->entries[lo-1].valueOrChild;

    // Recurse
    SplitResult childResult = insert(childPage, key, value);
    if (!childResult.promoteKey)
    {
        // Nothing bubbled up
        return result;
    }

    // Insert the promoted key/page into this node at index lo
    // Shift entries [lo..count) right by one
    if (lo < node->count)
    {
        std::memmove(&node->entries[lo+1],
            &node->entries[lo],(node->count - lo) * sizeof(BTreeNode::Entry));
    }
    node->entries[lo].key = childResult.promoteKey;
    node->entries[lo].valueOrChild = childResult.newPage;
    node->count++;

    // If fits, we're done
    if (node->count <= BTreeNode::MAX_ENTRIES) return result;

    // --- Split internal node ---
    uint32_t mid = node->count >> 1;  // choose split point
    uint32_t promoteKey = node->entries[mid].key;

    PageNum newPage = allocNode();
    BTreeNode* sibling = getNode(newPage);
    // The child pointer right of promoteKey becomes sibling's leftmostChild
    sibling->leftmostChild = node->entries[mid].valueOrChild;

    // Move entries [mid+1..count) into sibling.entries[0..]
    uint32_t moveCount = node->count - (mid + 1);
    if (moveCount > 0)
    {
        std::memcpy(&sibling->entries[0], &node->entries[mid+1],
            moveCount * sizeof(BTreeNode::Entry));
    }
    sibling->count = moveCount;

    // Shrink original node to [0..mid)
    node->count = mid;

    result.promoteKey  = promoteKey;
    result.newPage     = newPage;
    return result;
}

/// @brief Insert key/value into the B-tree rooted at rootPage.
///        If the root splits, this will allocate a new root and update rootPage.
/// @param rootPage   in/out: page ID of the current root (updated on split)
/// @param key        the key to insert
/// @param value      the value (or child pointer if used in a B+ tree context)
void BlobStore::Transaction::insert(PageNum* rootPage, uint32_t key, uint32_t value)
{
    SplitResult res = insert(*rootPage, key, value);
    if (res.promoteKey)
    {
        // Grow tree height: create new root
        PageNum newRootPage = allocNode();
        BTreeNode* newRoot = getNode(newRootPage);
        newRoot->leftmostChild = *rootPage;
        newRoot->entries[0].key = res.promoteKey;
        newRoot->entries[0].valueOrChild = res.newPage;
        newRoot->count = 1;
        *rootPage = newRootPage;
    }
}


/// @brief Remove and return an entry from the B-tree.
///
/// @param rootPage  in/out: page ID of the tree root
/// @param x         search key
/// @param exact     if true, remove only if key == x; if false, remove smallest key >= x
/// @return the value if an entry was found, otherwise 0
///
/// TODO: need to retrieve key/value, not just value !!!
uint32_t BlobStore::Transaction::take(PageNum* rootPage, uint32_t x, bool exact) noexcept
{
    // Path stacks (no dynamic alloc)
    BTreeNode* nodes[BTreeNode::MAX_HEIGHT];
    PageNum pages[BTreeNode::MAX_HEIGHT];
    uint32_t idxs [BTreeNode::MAX_HEIGHT];
    std::size_t depth = 0;

    // 1) Descend to find lower_bound
    PageNum page = *rootPage;
    while (page != 0 && depth < BTreeNode::MAX_HEIGHT)
    {
        BTreeNode* n = getNode(page);

        // binary search for first entry.key >= x
        uint32_t lo = 0, hi = n->count;
        while (lo < hi)
        {
            uint32_t mid = (lo + hi) >> 1;
            if (n->entries[mid].key < x)
            {
                lo = mid + 1;
            }
            else
            {
                hi = mid;
            }
        }

        nodes[depth] = n;
        pages[depth] = page;
        idxs [depth] = lo;
        ++depth;

        if (n->leftmostChild == 0) break;  // leaf reached

        page = (lo == 0) ? n->leftmostChild : n->entries[lo - 1].valueOrChild;
    }

    if (depth == 0) return 0;  // empty tree = no result

    BTreeNode* leaf = nodes[depth - 1];
    uint32_t i = idxs[depth - 1];

    // 2) Check if match exists
    if (i >= leaf->count || (exact && leaf->entries[i].key != x))
    {
        return 0;  // nothing to remove
    }

    // 3) Capture value and remove at leaf
    uint32_t outValue = leaf->entries[i].valueOrChild;
    std::memmove(&leaf->entries[i], &leaf->entries[i + 1],
        (leaf->count - i - 1) * sizeof(BTreeNode::Entry));
    --leaf->count;

    // 4) Rebalance upwards if underfull
    for (std::size_t level = depth - 1; level > 0; --level)
    {
        BTreeNode* n = nodes[level];
        uint32_t pid = pages[level];

        if (n->count >= BTreeNode::MIN_ENTRIES) break;

        // Identify parent and our index there
        BTreeNode* p = nodes[level - 1];
        uint32_t pp = pages[level - 1];
        uint32_t pidx = idxs[level - 1];

        // Sibling pages
        uint32_t leftPage  = (pidx == 0) ? 0 : p->entries[pidx - 1].valueOrChild;
        uint32_t rightPage = (pidx < p->count) ? p->entries[pidx].valueOrChild : 0;

        BTreeNode* leftSib  = leftPage  ? getNode(leftPage)  : nullptr;
        BTreeNode* rightSib = rightPage ? getNode(rightPage) : nullptr;

        // Try borrow from left
        if (leftSib && leftSib->count > BTreeNode::MIN_ENTRIES)
        {
            // shift n right
            std::memmove(&n->entries[1], &n->entries[0],
                n->count * sizeof(BTreeNode::Entry));
            // pull last of leftSib
            n->entries[0] = leftSib->entries[--leftSib->count];
            ++n->count;
            // update parent separator
            p->entries[pidx - 1].key = n->entries[0].key;
            continue;
        }
        // Try borrow from right
        if (rightSib && rightSib->count > BTreeNode::MIN_ENTRIES)
        {
            BTreeNode::Entry e = rightSib->entries[0];
            std::memmove(&rightSib->entries[0],
                         &rightSib->entries[1],
                         (rightSib->count - 1) * sizeof(BTreeNode::Entry));
            --rightSib->count;

            n->entries[n->count++] = e;
            p->entries[pidx].key   = rightSib->entries[0].key;
            continue;
        }
        // Merge
        if (leftSib)
        {
            // merge n into leftSib
            std::memcpy(&leftSib->entries[leftSib->count],
                        &n->entries[0],
                        n->count * sizeof(BTreeNode::Entry));
            leftSib->count += n->count;

            // remove separator at pidx-1
            std::memmove(&p->entries[pidx - 1],
                         &p->entries[pidx],
                         (p->count - pidx) * sizeof(BTreeNode::Entry));
            --p->count;
            freeNode(pid);
        }
        else if (rightSib)
        {
            // merge rightSib into n
            std::memcpy(&n->entries[n->count],
                        &rightSib->entries[0],
                        rightSib->count * sizeof(BTreeNode::Entry));
            n->count += rightSib->count;

            // remove separator at pidx
            std::memmove(&p->entries[pidx],
                         &p->entries[pidx + 1],
                         (p->count - pidx - 1) * sizeof(BTreeNode::Entry));
            --p->count;
            freeNode(rightPage);
        }
    }

    // 5) Collapse root if empty
    BTreeNode* root = getNode(*rootPage);
    if (root->leftmostChild != 0 && root->count == 0)
    {
        uint32_t newRoot = root->leftmostChild;
        freeNode(*rootPage);
        *rootPage = newRoot;
    }

    return outValue;
}


} // namespace clarisma
