// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/store/BlobStore2.h>
#include <clarisma/store/BTree.h>
#include <clarisma/util/Bits.h>
#include <clarisma/util/ShortVarString.h>

namespace clarisma {

class FreeTree : public BTree<FreeTree, BlobStore2::Transaction, uint32_t, uint32_t,4>
{
public:
    static size_t maxNodeSize(BlobStore2::Transaction* tx)
    {
        return 4096;    // TODO: Use true page size
    }

    static uint8_t* getNode(BlobStore2::Transaction* tx, NodeRef ref)    // CRTP override
    {
        return reinterpret_cast<uint8_t*>(tx->getPageBlock(ref));
    }

    static std::pair<NodeRef,uint8_t*> allocNode(BlobStore2::Transaction* tx)     // CRTP override
    {
        uint32_t page = tx->allocMetaPage();
        return { page, getNode(tx, page) };
    }

    static void freeNode(BlobStore2::Transaction* tx, NodeRef ref)     // CRTP override
    {
        tx->freeMetaPage(ref);
    }
};

class FreeSizeTree : public FreeTree
{
public:
    static uint32_t keyFromSize(uint32_t size)
    {
        return size; // TODO
    }

    static uint32_t keyFromSizeAndEnd(uint32_t size, uint32_t endPage)
    {
        return size; // TODO
    }

    static uint32_t sizeFromKey(uint32_t key)
    {
        return key; // TODO
    }

    class Cursor : public FreeTree::Cursor
    {
    public:
        using FreeTree::Cursor::Cursor;

        void moveToMinimumSize(uint32_t minSize)
        {
            moveToLowerBound(keyFromSize(minSize));
        }

        void insertSizeAndEnd(uint32_t size, uint32_t endPage)
        {
            insert(keyFromSizeAndEnd(size, endPage), endPage - size);
        }

        void removeSizeAndEnd(uint32_t size, uint32_t endPage)
        {
            remove(keyFromSizeAndEnd(size, endPage), endPage - size);
        }
    };
};



// TODO: must move JournalFile to store
//  because inside a CreateTransaction it is initialzied
//  before the Store itself is valid (it is a member of CreateTransaction,
//  therefore initialzied after base class Transaction)

DateTime BlobStore2::getLocalCreationTimestamp() const
{
	return getRoot()->creationTimestamp;
}

uint64_t BlobStore2::getTrueSize() const
{
    return static_cast<uint64_t>(getRoot()->totalPageCount) << pageSizeShift_;
}

// TODO: When does pageSize get set when creating a new BlobStore?
//  This affects the true size of the store
//  It must be set before Transaction::begin() is called
void BlobStore2::initialize(bool create)
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

uint32_t BlobStore2::pagesForPayloadSize(uint32_t payloadSize) const
{
    assert(payloadSize <= SEGMENT_LENGTH - BLOB_HEADER_SIZE);
    return (payloadSize + (1 << pageSizeShift_) + BLOB_HEADER_SIZE - 1) >> pageSizeShift_;
}


uint32_t BlobStore2::Transaction::allocMetaPage()
{
    Header* root = getRootBlock();
    uint32_t page = root->firstFreeMetaPage;
    if (page)
    {
        byte* p = getPageBlock(page);
        root->firstFreeMetaPage = *reinterpret_cast<uint32_t*>(p + 4);
        return page;
    }
    page = root->totalPageCount++;
    return page;
}

void BlobStore2::Transaction::freeMetaPage(uint32_t page)
{
    Header* root = getRootBlock();
    if (page == root->totalPageCount - 1)
    {
        // Page is at the end of the store -> shrink the store
        root->totalPageCount--;
        return;
    }
    byte* p = getPageBlock(page);
    *reinterpret_cast<uint32_t*>(p + 4) = root->firstFreeMetaPage;
    root->firstFreeMetaPage = page;
}

// TODO: mark segment as dirty, so that commit() will flush the mapping
//  (Can't rely on getBlock() anymore as we write directly into the
//  allocated space)

// TODO: size entry also includes hash!

uint32_t BlobStore2::Transaction::allocPages(uint32_t requestedPages)
{
    Header* root = getRootBlock();
    FreeSizeTree::Cursor bySize(this, &root->freeSizeTreeRoot);
    bySize.moveToMinimumSize(requestedPages);
    if (!bySize.isAfterLast())
    {
        // Found a free blob that is large enough
        FreeTree::Entry sizeEntry = bySize.entry();
        bySize.remove();
        uint32_t freePages = FreeSizeTree::sizeFromKey(sizeEntry.key);
        uint32_t firstPage = sizeEntry.value;
        FreeTree::Cursor byEnd(this, &root->freeEndTreeRoot);
        FreeTree::Entry* pEndEntry = byEnd.findExact(
            firstPage + requestedPages);
        assert(pEndEntry);

        // There must be an entry in the free-by-end index with
        // the exact key;
        // TODO: actual error handling in case
        //  of file corruption

        if (freePages == requestedPages)
        {
            // Perfect fit
            byEnd.remove();
            return firstPage;
        }

        // we need to give back the remaining chunk

        bySize.insertSizeAndEnd(
            freePages - requestedPages,
            firstPage + requestedPages);
        pEndEntry->value -= requestedPages;
        return firstPage;
    }

    uint32_t firstPage = root->totalPageCount;
    uint32_t pagesPerSegment = SEGMENT_LENGTH >> store()->pageSizeShift_;
    int remainingPages = pagesPerSegment - (firstPage & (pagesPerSegment - 1));
    if (remainingPages < requestedPages)
    {
        // If the blob won't fit into the current segment, we'll
        // mark the remaining space as a free blob, and allocate
        // the blob in a fresh segment

        root->totalPageCount += remainingPages;
            // We have to increase total pages before call to free(),
            // because free() may allocate one or more meta pages
        performFreePages(firstPage, remainingPages);
            // TODO: Name of this method may change; we want to actually
            //  free the blob now, not stage it
        firstPage = root->totalPageCount;
            // free() may increase the total page count due
            // to meta-page allocation
    }
    root->totalPageCount = firstPage + requestedPages;
    return firstPage;
}

void BlobStore2::Transaction::performFreePages(uint32_t firstPage, uint32_t pages)
{
    Header* root = getRootBlock();
    FreeSizeTree::Cursor bySize(this, &root->freeSizeTreeRoot);
    FreeTree::Cursor byEnd(this, &root->freeEndTreeRoot);

    if (firstPage + pages == root->totalPageCount)
    {
        // Blob is at end -> trim the file
        root->totalPageCount -= pages;
        for (;;)
        {
            byEnd.moveToLast();
            if (byEnd.isAfterLast()) return;
            FreeTree::Entry free = byEnd.entry();
            if (free.key != root->totalPageCount) return;
            root->totalPageCount -= free.value;
            bySize.removeSizeAndEnd(free.value, free.key);
            byEnd.remove();
        }
    }

    FreeTree::Entry* right = nullptr;
    FreeTree::Entry* left = nullptr;

    byEnd.moveToLowerBound(firstPage + 1);
    if (!byEnd.isAfterLast())
    {
        // Found a free blob after this blob
        // -> check if it is an immediate neighbor and
        // in current segment

        FreeTree::Entry* pEntry = byEnd.entryPtr();
        uint32_t rightSize = pEntry->value;
        uint32_t rightStart = pEntry->key - rightSize;
        if (rightStart == firstPage + pages &&
            !isFirstPageOfSegment(rightStart))
        {
            right = pEntry;
            pages += rightSize;
            right->value = pages;
            bySize.removeSizeAndEnd(rightSize, right->key);
        }
    }

    byEnd.movePrev();
    if (!byEnd.isBeforeFirst())
    {
        // Found a free blob before this blob
        // -> check if it is an immediate neighbor and
        // in current segment

        FreeTree::Entry* pEntry = byEnd.entryPtr();
        if (pEntry->key == firstPage && !isFirstPageOfSegment(firstPage))
        {
            // Found a left neighbor
            left = pEntry;
            uint32_t leftSize = pEntry->value;
            pages += leftSize;
            bySize.removeSizeAndEnd(left->value, left->key);

            if (right)
            {
                // If we also have a right neighbor, consolidate
                // the left neighbor into the right along with the
                // newly-freed blob
                right->value = pages;
                byEnd.remove();
                    // Caution: At this point, `neighbor` may no longer
                    // point to a valid position!
            }
            else
            {
                // consolidate the newly-freed blob into the
                // left neighbor
                left->value = pages;
            }
            firstPage = left->key - leftSize;
        }
    }
    if (!right && !left)
    {
        // No consolidation into existing end-entry,
        // so we'll have to create a new one
        byEnd.insertAfterCurrent(firstPage + pages, pages);
    }

    // Create the new size-entry
    bySize.insertSizeAndEnd(pages, firstPage + pages);
}


void BlobStore2::Transaction::commit()
{
    for (const auto& [firstPage, pages] : freedBlobs_)
    {
        performFreePages(firstPage, pages);
    }

    Store::Transaction::commit();

    for (const auto& [firstPage, pages] : freedBlobs_)
    {
        uint64_t ofs = store()->offsetOf(firstPage);
        uint64_t size = store()->offsetOf(pages);
        store()->deallocate(ofs, size);
        // TODO: Is it possible that the physical block size
        //  is larger than the page size? If so, must align
        //  the "hole"
    }
}

void BlobStore2::Transaction::beginCreateStore()
{

}

void BlobStore2::Transaction::endCreateStore()
{

}

#ifdef OLD_XXX

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
BlobStore2::PageNum BlobStore2::Transaction::alloc(uint32_t payloadSize)
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

#endif

} // namespace clarisma
