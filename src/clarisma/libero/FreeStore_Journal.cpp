// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/libero/FreeStore_Journal.h>
#include <cassert>
#include <clarisma/util/log.h>
#include <clarisma/util/Pointers.h>

namespace clarisma {

void FreeStore::Journal::open(const char* fileName, uint32_t bufSize)
{
    assert(bufSize >= 2 * BLOCK_SIZE);
    file_.open(fileName, File::OpenMode::CREATE | File::OpenMode::WRITE);
    buf_.reset(new byte[bufSize]);
    end_ = buf_.get() + bufSize;
}

void FreeStore::Journal::close()
{
    file_.tryClose();
}


void FreeStore::Journal::reset(uint64_t marker, const Header* header)
{
    filePos_ = 0;
    *reinterpret_cast<uint64_t*>(buf_.get()) = marker;
    memcpy(buf_.get() + 8, header, HEADER_SIZE);
    bufPos_ = HEADER_SIZE + 8;
    checksum_ = Crc32C();
}

void FreeStore::Journal::addBlock(uint64_t marker, const void* content, size_t size)
{
    assert(size <= BLOCK_SIZE);
    byte* p = buf_.get() + bufPos_;
    *reinterpret_cast<uint64_t*>(p) = marker;
    bufPos_ += 8;
    p += 8;
    if (p + size >= end_)
    {
        auto remaining = Pointers::nearDelta(end_, p);
        memcpy(p, content, remaining);
        bufPos_ += remaining;
        computeChecksum();
        writeToFile();
        size -= remaining;
    }
    memcpy(buf_.get(), content, size);
    bufPos_ = size;
}

void FreeStore::Journal::seal()
{
    computeChecksum();
    *reinterpret_cast<uint64_t*>(buf_.get() + bufPos_) =
        JOURNAL_END_MARKER_FLAG | checksum_.get();
    bufPos_ += 8;
    writeToFile();
    file_.syncData();
}


void FreeStore::Journal::computeChecksum()
{
    checksum_.update(buf_.get(), bufPos_);
}

void FreeStore::Journal::writeToFile()
{
    file_.writeAllAt(filePos_, buf_.get(), bufPos_);
    filePos_ += bufPos_;
    bufPos_ = 0;
}

} // namespace clarisma
