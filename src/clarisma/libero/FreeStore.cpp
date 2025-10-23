// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/libero/FreeStore_Journal.h>
#include <clarisma/util/Crc32C.h>
#include <clarisma/util/Strings.h>
#include <cassert>

#include "clarisma/io/MemoryMapping.h"
#include "clarisma/util/StringBuilder.h"

namespace clarisma {

void FreeStore::open(const char* fileName, OpenMode mode)
{
    File file;
    MemoryMapping mapping;
    bool lockedExclusively = false;
    bool writable = hasAny(mode,OpenMode::WRITE | OpenMode::CREATE);
    bool created = false;
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
    journalFileName_ = Strings::combine(fileName, ".journal");

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
                    throw FreeStoreException("Store locked");
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
                    throw FreeStoreException("Store locked");
                }
                lockStart = LOCK_OFS + 1;
                lockSize = 1;
            }
            uint64_t size = file.size();
            if (size >= HEADER_SIZE) [[likely]]
            {
                mapping = MemoryMapping(file, 0, size);
            }
            else if (size == 0)
            {
                created = true;
                break;
            }
            else
            {
                throw FreeStoreException("Invalid store");
            }
        }
        else
        {
            // Open-for non-exclusive reading: we have to read
            // the active snapshot indicator in order to
            // figure out which snapshot to lock, the read
            // header again and check if changed in the meantime

            BasicHeader basicHeader;
            (void)file.tryReadAllAt(0, &basicHeader, sizeof(BasicHeader));
            for (;;)
            {
                lockStart = LOCK_OFS + (basicHeader.activeSnapshot << 1);
                lockSize = 1;
                if (!file.tryLockShared(lockStart, 1))
                {
                    throw FreeStoreException("Store locked");
                }
                uint64_t size = file.size();
                if (size < HEADER_SIZE)
                {
                    throw FreeStoreException("Invalid store");
                }
                mapping = MemoryMapping(file, 0, size);
                if (reinterpret_cast<const Header*>(
                    mapping.data())->commitId == basicHeader.commitId)
                {
                    break;
                }
                mapping.unmap();
                file.tryUnlock(lockStart, 1);
            }
        }

        int res = ensureIntegrity(fileName, file,
            reinterpret_cast<const HeaderBlock*>(mapping.data()),
            journalFileName_.c_str(), writable);
        if (res == 0)  [[likely]]
        {
            break;
        }
        if (res == -1)
        {
            // Retry
            // TODO: delay
        }
        else
        {
            assert(res == 1 || res == 2);
            // Always start over after rollback, because header state
            // may have changed (including active snapshot).
            // Moreover, closing the second (writable) file handle
            // causes locks to be dropped on Posix, so we always
            // re-acquire the lock

            if (res == 2)
            {
                // Incomplete existing file, truncate it (without
                // affecting locks) and treat it as newly created
                mapping.unmap();
                (void)file.trySetSize(4096);
                created = true;
                break;
            }
        }
        file.tryUnlock(lockStart, lockSize);
    }

    if (!created) [[likely]]
    {
        initialize(mapping.data());
    }

    file_ = std::move(file);
    fileName_ = fileName;
    writeable_ = writable;
    lockedExclusively_ = lockedExclusively;
    created_ = created;
    mapping_ = std::move(mapping);
}

void FreeStore::close()
{
    file_.close();
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

    Crc32C crc;
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
    std::span<const byte> journal)
{
    HeaderBlock header;
    const byte* p = journal.data() + sizeof(uint64_t);
    memcpy(&header, p, HEADER_SIZE);
    p += HEADER_SIZE;

    for (;;)
    {
        uint64_t ofs = *reinterpret_cast<const uint64_t*>(p);
        if (ofs & JOURNAL_END_MARKER_FLAG) break;
        p += sizeof(uint64_t);
        writableStore.writeAllAt(ofs, p, BLOCK_SIZE);
        p += BLOCK_SIZE;
    }
    writableStore.sync();

    // After applying the Journal, we inc the commit ID and
    // write the header in a separate sync phase (just like
    // a commit in a transaction)
    // This prevents the Journal form being re-applied in case
    // we're unable to delete it

    ++header.commitId;
    Crc32C crc;
    crc.update(&header, CHECKSUMMED_HEADER_SIZE);
    header.checksum = crc.get();
    writableStore.writeAllAt(0, &header, HEADER_SIZE);
    writableStore.sync();
}

int FreeStore::ensureIntegrity(
    const char* storeFileName, FileHandle storeHandle,
    const HeaderBlock* header, const char* journalFileName, bool isWriter)
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
                // TODO: Move this into separate, customizable method

                if (header->magic == 0)     // blank slate
                {
                    return isWriter ? 2 : 0;
                }
                if (header->magic == 0x7ADA0BB1)     // v1 format
                {
                    throw FreeStoreException("Unsupported Store Format (Version 1.0)");
                }
                if (header->magic == 0x1CE50D6E)     // v2 format, but invalid
                {
                    char buf[1024];
                    Crc32C crc;
                    crc.update(header, CHECKSUMMED_HEADER_SIZE);
                    Format::unsafe(buf,
                        "Store corrupted: Header checksum mismatch "
                        "(%08X vs %08X)", header->checksum, crc.get());
                    throw FreeStoreException(buf);
                }
                throw FreeStoreException("Unrecognized file format");
            }
            return 0;
        }
        throw IOException();
    }

    // TODO: just memmap here (but reject journal if not min length)
    //  (Don't map a 0-length file)

    struct
    {
        uint64_t journalMode;
        BasicHeader header;
    }
    journalHeader;
    // TODO: init fields
    (void)journalFile.tryReadAll(&journalHeader, sizeof(journalHeader));

    File writableStoreFile;
    FileHandle writableStoreHandle = storeHandle;
    if (!isWriter)
    {
        // Attempt to lock the store file for writing
        if (!storeHandle.tryLock(LOCK_OFS + 1,1))
        {
            // TODO: handle other possible locking errors
            journalFile.close();
            if (!isHeaderValid || journalHeader.journalMode == Journal::MODIFIED_ALL)
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

    int result = 0;
    size_t journalSize = journalFile.size();
    if (journalSize > 0)
    {
        MemoryMapping journal = { journalFile, 0, journalSize };
        if (journalHeader.header.commitId == header->commitId || !isHeaderValid)
        {
            if (verifyJournal(journal))
            {
                applyJournal(writableStoreHandle, journal);
                result = 1;
            }
        }
        journal.unmap();
    }
    journalFile.tryClose();
    std::remove(journalFileName);

    if (!isWriter)
    {
        storeHandle.tryUnlock(LOCK_OFS + 1,1);
    }

    // mapping & files are auto-closed
    return result;
}

} // namespace clarisma
