// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/libero/FreeStore.h>
#include <clarisma/util/Crc32.h>
#include <clarisma/util/enum.h>
#include <cassert>

#include "clarisma/io/MemoryMapping.h"

namespace clarisma {

void FreeStore::open(const char* fileName, OpenMode mode)
{
    File file;
    HeaderBlock header;
    bool lockedExclusively = false;
    bool writable = hasAny(mode,OpenMode::WRITE | OpenMode::CREATE);
    int snapshot;
    int lockStart;
    int lockSize;

    File::OpenMode writeMode =
        has(mode, OpenMode::WRITE) ? File::OpenMode::WRITE :
            static_cast<File::OpenMode>(0);
    File::OpenMode createMode =
        has(mode, OpenMode::CREATE) ?
            (File::OpenMode::CREATE | File::OpenMode::WRITE |
                File::OpenMode::SPARSE) : static_cast<File::OpenMode>(0);

    file.open(fileName, File::OpenMode::READ | writeMode | createMode);
    
    for (;;)
    {
        if (hasAny(mode,OpenMode::EXCLUSIVE | OpenMode::TRY_EXCLUSIVE |
            OpenMode::WRITE | OpenMode::CREATE)) [[ unlikely]]
        {
            if (hasAny(mode, OpenMode::EXCLUSIVE | OpenMode::TRY_EXCLUSIVE))
            {
                if (!file.tryLock(LOCK_OFS, 3), !writable)
                {
                    if (has(mode, OpenMode::TRY_EXCLUSIVE))
                    {
                        mode &= ~(OpenMode::EXCLUSIVE | OpenMode::TRY_EXCLUSIVE);
                        continue;
                    }
                    throw StoreException("Store locked");
                }
                lockedExclusively = true;
                lockStart = LOCK_OFS;
                lockSize = 3;
            }
            else
            {
                // Try to lock for writing
                if (!file.tryLock(LOCK_OFS + 1, 1), false)
                {
                    throw StoreException("Store locked");
                }
                lockStart = LOCK_OFS + 1;
                lockSize = 1;
            }
            file.readAllAt(0, &header, sizeof(header));
        }
        else
        {
            // Open-for non-exclusive reading: we have to read
            // the active snapshot indicator in order to
            // figure out which snapshot to lock, the read
            // header again and check if changed in the meantime

            file.readAllAt(0, &header, sizeof(BasicHeader));
            for (;;)
            {
                uint64_t commitId = header.commitId;
                lockStart = LOCK_OFS + (header.activeSnapshot << 1);
                lockSize = 1;
                if (!file.tryLockShared(lockStart, 1))
                {
                    throw StoreException("Store locked");
                }
                file.readAllAt(0, &header, sizeof(header));
                if (header.commitId == commitId) break;
                file.tryUnlock(lockStart, 1);
            }
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
            // Always start over after rollback, because header state
            // may have changed (including active snapshot).
            // Moreover, closing the second (writable) file handle
            // causes locks to be dropped on Posix, so we always
            // re-acquire the lock
        }
        file.tryUnlock(lockStart, lockSize);
    }

    file_ = std::move(file);
    writeable_ = writable;
    lockedExclusively_ = lockedExclusively;
}

void FreeStore::close()
{
    file_.close();
}

// TODO: remove
void FreeStore::open(const char* fileName)
{
    File file;
    file.open(fileName, File::OpenMode::READ);
    HeaderBlock header;
    read_txid:
    file.readAll(&header, sizeof(BasicHeader));
    for (;;)
    {
        uint64_t commitId = header.commitId;
        int snapshot;
        for (;;)
        {
            snapshot = header.activeSnapshot;
            if (!file.tryLockShared(LOCK_OFS + (snapshot << 1), 1))
            {
                throw StoreException("Store locked for updates");
            }
            file.readAllAt(0, &header, sizeof(header));
            if (header.commitId == commitId) break;
            file.tryUnlock(LOCK_OFS + (snapshot << 1), 1);
            commitId = header.commitId;
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
            // Always start over after rollback, because header state
            // may have changed (including active snapshot).
            // Moreover, closing the second (writable) file handle
            // causes locks to be dropped on Posix, so we always
            // re-acquire the lock
        }
        file.tryUnlock(LOCK_OFS + (snapshot << 1), 1);
    }
}

bool FreeStore::verifyHeader(const HeaderBlock* header)
{
    Crc32C crc;
    crc.update(header, CHECKSUMMED_HEADER_SIZE);
    return crc.get() == header->checksum;
}

// TODO: This assumes it is legal for the Journal to have extra bytes
//  after the trailer (allows Writer to replace without trim after each
//  commit, which is slightly faster; validity is still guaranteed
//  via checksum)
bool FreeStore::verifyJournal(std::span<const byte> journal)
{
    if (journal.size() < HEADER_SIZE + 2 * sizeof(uint64_t))
    {
        // Minimum journal: journal_mode + header + trailer
        return false;
    }

    const byte* p = journal.data();
    const byte* dataEnd = p + journal.size() - sizeof(uint64_t);

    Crc32 crc;
    crc.update(p, HEADER_SIZE + sizeof(uint64_t));
    p += HEADER_SIZE + sizeof(uint64_t);

    uint64_t ofs;
    for (;;)
    {
        ofs = *reinterpret_cast<const uint64_t*>(p);
        if (ofs & JOURNAL_END_MARKER_FLAG) break;
        const byte* blockEnd = p + BLOCK_SIZE + sizeof(uint64_t);
        if (blockEnd > dataEnd) return false;
        if (ofs % BLOCK_SIZE != 0) return false;
        crc.update(p, BLOCK_SIZE + sizeof(uint64_t));
        p = blockEnd;
    }

    return crc.get() == *reinterpret_cast<const uint32_t*>(p);
}

// Journal must be valid
// NOLINTNEXTLINE: not const
void FreeStore::applyJournal(FileHandle writableStore,
    std::span<const byte> journal, HeaderBlock* header)
{
    const byte* p = journal.data() + sizeof(uint64_t);
    memcpy(header, p, HEADER_SIZE);
    p += HEADER_SIZE;

    for (;;)
    {
        uint64_t ofs = *reinterpret_cast<const uint64_t*>(p);
        if (ofs & JOURNAL_END_MARKER_FLAG) break;
        p += sizeof(uint64_t);
        writableStore.writeAllAt(ofs, p, BLOCK_SIZE);
        p += BLOCK_SIZE;
    }

    writableStore.writeAllAt(0, header, HEADER_SIZE);
    writableStore.sync();
}

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

    struct
    {
        uint64_t journalMode;
        BasicHeader header;
    }
    journalHeader;
    journalFile.readAll(&journalHeader, sizeof(journalHeader));

    File writableStoreFile;
    FileHandle writableStoreHandle = storeHandle;
    if (!isWriter)
    {
        // Attempt to lock the store file for writing
        if (!storeHandle.tryLock(LOCK_OFS + 1,1))
        {
            // TODO: handle other possible locking errors
            journalFile.close();
            if (!isHeaderValid || header->commitId == journalHeader.header.commitId)
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
        applyJournal(writableStoreHandle, journal, header);
    }

    if (!isWriter)
    {
        storeHandle.tryUnlock(LOCK_OFS + 1,1);
    }

    // mapping & files are auto-closed
    return 1;
}

} // namespace clarisma
