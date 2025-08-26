// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/libero/FreeStore.h>
#include <clarisma/util/Crc32.h>
#include <cassert>

#include "clarisma/io/MemoryMapping.h"

namespace clarisma {

void FreeStore::open(const char* fileName)
{
    File file;
    file.open(fileName, File::OpenMode::READ);
    HeaderBlock header;
    read_txid:
    file.readAll(&header, sizeof(BasicHeader));
    for (;;)
    {
        uint64_t txId = header.transactionId;
        int snapshot;
        for (;;)
        {
            snapshot = txId & 1;
            if (!file.tryLockShared(snapshot << 1, 1))
            {
                throw StoreException("Store locked for updates");
            }
            file.readAllAt(0, &header, sizeof(header));
            if (header.transactionId == txId) break;
            file.tryUnlock(snapshot << 1, 1);
            txId = header.transactionId;
        }

        std::string journalFileName = getJournalFileName();
        int res = ensureIntegrity(
            fileName, file, &header,
            journalFileName.c_str(), false);
        if (res == 0)  [[likely]]
        {
            break;
        }
        if (res == -1)
        {
            // TODO: delay
            file.readAllAt(0, &header, sizeof(BasicHeader));
        }
        else
        {
            assert(res == 1);
            if (header.transactionId == txId) break;
            // Journal rollback may have changed snapshot
        }
        file.tryUnlock(snapshot << 1, 1);
    }
}

bool FreeStore::verifyHeader(HeaderBlock* header)
{
    uint32_t checksum = header->checksum;
    header->checksum = 0;
    Crc32 crc;
    crc.update(header, sizeof(HeaderBlock));
    header->checksum = checksum;
    return crc.get() == checksum;
}

void FreeStore::sealHeader(HeaderBlock* header)
{
    header->checksum = 0;
    Crc32 crc;
    crc.update(header, sizeof(HeaderBlock));
    header->checksum = crc.get();
}



// TODO: This assumes it is legal for the Journal to have extra bytes
//  after the trailer (allows Writer to replace without trim after each
//  commit, which is slightly faster; validity is still guaranteed
//  via checksum)
bool FreeStore::verifyJournal(std::span<const byte> journal)
{
    if (journal.size() < 24)
    {
        // Minimum journal: header + trailer
        return false;
    }

    const byte* p = journal.data();
    const byte* trailer = p + journal.size() - 16;

    Crc32 crc;
    crc.update(p, 8);
    p+= 8;

    uint64_t ofs;
    for (;;)
    {
        ofs = *reinterpret_cast<const uint64_t*>(p);
        if (ofs & JOURNAL_END_MARKER_FLAG) break;
        const byte* blockEnd = p + BLOCK_SIZE + 8;
        if (blockEnd > trailer) return false;
        if (ofs % BLOCK_SIZE != 0) return false;
        crc.update(p, BLOCK_SIZE + 8);
        p = blockEnd;
    }

    crc.update(p, 8);
    p += 8;
    return crc.get() == *reinterpret_cast<const uint64_t*>(p);
}

// Journal must be valid
// TODO: guard against over-read anyway
// TODO: What if header is invalid, but journal does not contain header?
// NOLINTNEXTLINE: not const
void FreeStore::applyJournal(FileHandle writableStore,
    std::span<const byte> journal,
    HeaderBlock* header, bool isHeaderValid)
{
    const byte* p = journal.data() + 8;

    bool journalHasHeader = false;
    uint64_t ofs;
    for (;;)
    {
        ofs = *reinterpret_cast<const uint64_t*>(p);
        if (ofs & JOURNAL_END_MARKER_FLAG) break;
        p += 8;
        if (ofs == 0)
        {
            memcpy(header, p, BLOCK_SIZE);
            journalHasHeader = true;
        }
        else
        {
            writableStore.writeAllAt(ofs, p, BLOCK_SIZE);
        }
        p += BLOCK_SIZE;
    }

    if (!isHeaderValid && !journalHasHeader)
    {
        throw StoreException("Failed to recover header from journal");
    }

    header->dirtyAllocFlag = static_cast<uint32_t>(ofs);
    sealHeader(header);
    writableStore.writeAllAt(0, header, BLOCK_SIZE);
    writableStore.sync();
}

// need store file name
int FreeStore::ensureIntegrity(
    const char* storeFileName, FileHandle storeHandle,
    HeaderBlock* header, const char* journalFileName, bool isWriter)
{
    bool isHeaderValid = verifyHeader(header);
    File journalFile;
    if (!journalFile.tryOpen(journalFileName, File::OpenMode::READ))
    {
        FileError error = File::error();
        if (error == FileError::NOT_FOUND)
        {
            if (!isHeaderValid)
            {
                throw StoreException("Missing journal; unable to restore header");
            }
            return 0;
        }
        throw IOException();
    }

    uint64_t journalTransactionId;
    journalFile.readAll(&journalTransactionId, sizeof(journalTransactionId));

    File writableStoreFile;
    FileHandle writableStoreHandle = storeHandle;
    if (!isWriter)
    {
        // Attempt to lock the store file for writing
        if (!storeHandle.tryLock(1,1))
        {
            // TODO: handle other possible locking errors
            journalFile.close();
            if (!isHeaderValid || header->transactionId == journalTransactionId)
            {
                // Journal locked, retry
                return -1;
            }
            return 0;
                // skip journal processing, because either a Writer is
                // active, or another Reader is applying the Journal
                // for a previous tx
        }
        writableStoreFile.open(storeFileName, FileHandle::OpenMode::WRITE);
        writableStoreHandle = writableStoreFile;
    }

    size_t journalSize = journalFile.size();
    byte* mappedJournal = journalFile.map(0, journalSize);
    MemoryMapping journal = { mappedJournal, journalSize };
    if (verifyJournal(journal))
    {
        applyJournal(writableStoreHandle, journal, header, isHeaderValid);
    }

    if (!isWriter)
    {
        storeHandle.tryUnlock(1,1);
    }

    // mapping & files are auto-closed
    return 1;
}

} // namespace clarisma
