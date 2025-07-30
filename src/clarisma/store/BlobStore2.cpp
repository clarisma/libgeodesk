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
            bool success = remove(keyFromSizeAndEnd(size, endPage), endPage - size);
            if (!success)
            {
                transaction()->dumpFreePages();
                // try again for debugging
                remove(keyFromSizeAndEnd(size, endPage), endPage - size);
            }
            assert(success);
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
        if(root->magic != 0 || !create)
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
        // LOGS << "Taking meta page from free list\n";

        byte* p = getPageBlock(page);
        root->firstFreeMetaPage = *reinterpret_cast<uint32_t*>(p + 4);
        return page;
    }
    LOGS << "Creating new meta page\n";

    page = root->totalPageCount++;
    return page;
}

void BlobStore2::Transaction::freeMetaPage(uint32_t page)
{
    Header* root = getRootBlock();
    if (page == root->totalPageCount - 1)
    {
        LOGS << "Discarding meta page\n";

        // Page is at the end of the store -> shrink the store
        root->totalPageCount--;
        return;
    }

    // LOGS << "Adding meta page to free list\n";

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
    assert(requestedPages > 0);
    assert(requestedPages <= (SEGMENT_LENGTH >> store()->pageSizeShift_));

    // checkFreeTrees();
    // LOGS << "Allocating " << requestedPages << " pages\n";

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
            firstPage + freePages);
        if (!pEndEntry)
        {
            LOGS << "End table does not contain free blob at "
                << firstPage << " (" << freePages << " pages)\n";
            dumpFreePages();
        }
        assert(pEndEntry);

        // There must be an entry in the free-by-end index with
        // the exact key;
        // TODO: actual error handling in case
        //  of file corruption

        if (freePages == requestedPages)
        {
            /*
            LOGS << "  Found perfect fit of " << requestedPages
                << " pages at " << firstPage;
            */

            // Perfect fit
            byEnd.remove();
            return firstPage;
        }

        // we need to give back the remaining chunk

        /*
        LOGS << "  Allocated " << requestedPages
            << " pages at " << firstPage << ", giving back "
            << (freePages - requestedPages) << " at " <<
                (firstPage + requestedPages) << "\n";
        */

        bySize.insertSizeAndEnd(
            freePages - requestedPages,
            firstPage + freePages);
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

        uint32_t firstRemainingPage = root->totalPageCount;
        firstPage = firstRemainingPage + remainingPages;
        root->totalPageCount = firstPage + requestedPages;
            // We have to increase total pages before call to free(),
            // because performFreePages() may allocate one or more
            // meta pages
            // We have to increase total pages beyond the file end,
            // otherwise performFreePages() will simply trim it back
            // again rather than marking the free range
            // Also be aware that performFreePages() may increase
            // the total page count due to meta-page allocation
        performFreePages(firstRemainingPage, remainingPages);

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

    root->totalPageCount = firstPage + requestedPages;
    return firstPage;
}

void BlobStore2::Transaction::performFreePages(uint32_t firstPage, uint32_t pages)
{
    assert(pages > 0);
    assert(pages <= (SEGMENT_LENGTH >> store()->pageSizeShift_));

    // checkFreeTrees();
    // LOGS << firstPage << ": Freeing " << pages << " pages\n";

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
            LOGS << (free.key- free.value) << ": Trimmed " << free.value << " from end";
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

            /*
            LOGS << "  Found right neighbor at " << rightStart <<
                " (" << rightSize << " pages)\n";
            */

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

            /*
            LOGS << "  Found left neighbor at " << (left->key-leftSize) <<
                " (" << leftSize << " pages)\n";
            */

            pages += leftSize;
            bySize.removeSizeAndEnd(leftSize, left->key);

            if (right)
            {
                // If we also have a right neighbor, consolidate
                // the left neighbor into the right along with the
                // newly-freed blob
                right->value = pages;
                    // Caution: At this point, `neighbor` may no longer
                    // point to a valid position!

                // LOGS << "  Merging left and new block into right\n";
            }
            else
            {
                // LOGS << "  Merging left block into new\n";
            }

            // We always remove the end-entry of the left neighbor
            // since we cannot consolidate into it without changing
            // its key

            firstPage -= leftSize;
            byEnd.remove();     // remove left
        }
    }
    if (!right)
    {
        // No consolidation into existing right end-entry,
        // so we'll have to create a new one
        byEnd.insert(firstPage + pages, pages);
    }

    // Create the new size-entry
    bySize.insertSizeAndEnd(pages, firstPage + pages);
}


void BlobStore2::Transaction::commit()
{
    for (const auto& [firstPage, pages] : freedBlobs_)
    {
        performFreePages(firstPage, pages);
        // checkFreeTrees();
    }

    Store::Transaction::commit();

    /*
    for (const auto& [firstPage, pages] : freedBlobs_)
    {
        uint64_t ofs = store()->offsetOf(firstPage);
        uint64_t size = store()->offsetOf(pages);
        store()->deallocate(ofs, size);
        // TODO: Is it possible that the physical block size
        //  is larger than the page size? If so, must align
        //  the "hole"
    }
    */

    freedBlobs_.clear();
}

void BlobStore2::Transaction::beginCreateStore()
{
    Header* header = reinterpret_cast<Header*>(unjournaledDataPtr(0).ptr());
        // TODO: simply get the main mapping?
    memset(header, 0, sizeof(Header));
    header->creationTimestamp = DateTime::now();

    // Magic & version is set by the subclass
}

void BlobStore2::Transaction::endCreateStore()
{
    Header* header = reinterpret_cast<Header*>(unjournaledDataPtr(0).ptr());
    // TODO: simply get the main mapping?
    header->totalPageCount = 1;
    FreeSizeTree::init(this, &header->freeSizeTreeRoot);
    FreeTree::init(this, &header->freeEndTreeRoot);

    // TODO: make this cleaner
    preCommitStoreSize_ = 4096;
    Header* journaledHeader = getRootBlock();
    journaledHeader->magic = MAGIC;    // TODO
}

void BlobStore2::Transaction::dumpFreePages()
{
    Header* root = getRootBlock();

    size_t sizeSize = 0;
    size_t sizeCount = 0;
    FreeSizeTree::Iterator iter(this, &root->freeSizeTreeRoot);
    LOGS << "Free pages by size:\n";
    while (iter.hasNext())
    {
        FreeTree::Entry entry = iter.next();
        LOGS << "- " << entry.value << ": " <<
            FreeSizeTree::sizeFromKey(entry.key) << '\n';
        sizeCount++;
        sizeSize += FreeSizeTree::sizeFromKey(entry.key);
    }
    LOGS << "  " << sizeCount << " entries with " << sizeSize << " total pages\n";

    size_t endSize = 0;
    size_t endCount = 0;

    FreeTree::Iterator iter2(this, &root->freeEndTreeRoot);
    LOGS << "Free pages by location:\n";
    while (iter2.hasNext())
    {
        FreeTree::Entry entry = iter2.next();
        LOGS << "- " << (entry.key - entry.value) << ": " << entry.value << '\n';
        endCount++;
        endSize += entry.value;
    }
    LOGS << "  " << endCount << " entries with " << endSize << " total pages\n";
    LOGS << root->totalPageCount << " total pages\n";
    LOGS << "Free ratio: " << (static_cast<double>(endSize) / root->totalPageCount) << "\n";
    assert(endCount == sizeCount);
    assert(endSize == sizeSize);
}

struct Free
{
    uint32_t firstPage;
    uint32_t pages;

    bool operator==(const Free& other) const = default;

    bool operator<(const Free& other) const
    {
        return firstPage < other.firstPage;
    }
};

void dumpFreeEntries(const char* name, std::vector<Free>& list)
{
    LOGS << "Free entries (" << name << "):\n";
    for (const auto& entry : list)
    {
        LOGS << "- " << entry.firstPage << ": " << entry.pages << "\n";
    }
}

void BlobStore2::Transaction::checkFreeTrees()
{
    std::vector<Free> inSize;
    std::vector<Free> inEnd;

    Header* root = getRootBlock();
    FreeSizeTree::Iterator bySize(this, &root->freeSizeTreeRoot);
    while (bySize.hasNext())
    {
        const auto [k,v] = bySize.next();
        inSize.emplace_back(v, FreeSizeTree::sizeFromKey(k));
    }
    FreeTree::Iterator byEnd(this, &root->freeEndTreeRoot);
    while (byEnd.hasNext())
    {
        const auto [k,v] = byEnd.next();
        inEnd.emplace_back(k-v, v);
    }

    std::sort(inSize.begin(), inSize.end());
    if (inSize != inEnd)
    {
        LOGS << "FreeBlob trees don't match.\n";
        dumpFreeEntries("by size", inSize);
        dumpFreeEntries("by end", inEnd);
        assert(false);
    }
}


} // namespace clarisma
