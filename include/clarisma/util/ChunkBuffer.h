// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/data/Chunk.h>
#include <clarisma/util/Buffer.h>

namespace clarisma {

class ChunkBuffer : public Buffer
{
public:
    ChunkBuffer(size_t size) :
        chain_(size)
    {
        useChunk(chain_.first());
    }

    void filled(char* p) override
    {
        assert(p >= buf_);
        assert(p <= end_);
        Chunk<char>* chunk = Chunk<char>::ptrFromData(buf_);
        size_t capacity = chunk->size();
        chunk->trim(p_ - buf_);
        Chunk<char>* newChunk = Chunk<char>::create(capacity);
        chunk->setNext(newChunk);
        useChunk(newChunk);
    }

    void flush(char* p) override
    {
        assert(p >= buf_);
        assert(p <= end_);
        p_ = p;
    }

    ChunkChain<char> take()
    {
        buf_ = p_ = end_ = nullptr;
        return std::move(chain_);
    }

    ChunkChain<char> takeAndReplace(size_t size)
    {
        ChunkChain<char> old(std::move(chain_));
        chain_ = ChunkChain<char>(size);
        useChunk(chain_.first());
        return std::move(old);
    }

protected:
    void useChunk(Chunk<char>* chunk)
    {
        buf_ = chunk->data();
        p_ = buf_;
        end_ = buf_ + chunk->size();
    }
    ChunkChain<char> chain_;
};

} // namespace clarisma
