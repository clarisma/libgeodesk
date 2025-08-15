// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/libero/FreeStore.h>
#include <clarisma/util/Crc32.h>
#include <cassert>

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
        for (;;)
        {
            int snapshot = txId & 1;
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
        int res = ensureIntegrity(file, &header, journalFileName.c_str(), false);
        if (res == 0)  [[likely]]
        {
            break;
        }
        if (res == -1)
        {
            // TODO: delay
            file.readAllAt(0, &header, sizeof(BasicHeader));
        }
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
void FreeStore::applyJournal(std::span<const byte> journal,
    HeaderBlock* header, bool isHeaderValid)
{
    File writableStore;
    writableStore.open(fileName_.c_str(), File::OpenMode::WRITE);

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
int FreeStore::ensureIntegrity(FileHandle storeFile, HeaderBlock* header,
    const char* journalFileName, bool isWriter)
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
        throw IOException(error);
    }

    uint64_t journalTransactionId;
    journalFile.readAll(&journalTransactionId, sizeof(journalTransactionId));

    FileHandle writableStoreFile = storeFile;
    if (!isWriter)
    {
        // Attempt to lock the store file for writing
        if (!storeFile.tryLock(1,1))
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
        // writableStoreFile.open( = storeFile
    }


    // TODO
}


} // namespace clarisma
