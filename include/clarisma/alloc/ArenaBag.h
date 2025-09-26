// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/alloc/Arena.h>

namespace clarisma {

template <typename T, size_t CHUNK_SIZE>
class ArenaBag
{
private:
    struct Chunk
    {
        Chunk(Chunk* next) : next(next) {}

        Chunk* next;
        T items[CHUNK_SIZE];
    };

public:
    explicit ArenaBag(Arena& arena) :
        last_(nullptr),
        first_(nullptr),
        totalCount_(0),
        arena_(arena)
    {
    }

    bool isEmpty() const { return totalCount_ == 0; }

    size_t size() const { return totalCount_; }

    void add(T item)
    {
        size_t n = totalCount_ % CHUNK_SIZE;
        if (n == 0)    [[unlikely]]
        {
            Chunk* chunk = arena_.create<Chunk>(nullptr);
            if (last_)
            {
                assert(first_);
                last_->next = chunk;
            }
            else
            {
                first_ = chunk;
            }
            last_ = chunk;
        }
        last_->items[n] = item;
        totalCount_++;
    }

    class Iterator
    {
    public:
        explicit Iterator(const ArenaBag* bag) :
            chunk_(bag->first_),
            pos_(0),
            remaining_(bag->totalCount_) {}

        const T& operator*() const noexcept
        {
            return chunk_->items[pos_];
        }

        T* operator->() const noexcept
        {
            return &chunk_->items[pos_];
        }

        // Increment operator
        Iterator& operator++()
        {
            remaining_--;
            pos_++;
            if(pos_ == CHUNK_SIZE)    [[unlikely]]
            {
                chunk_ = chunk_->next;
                pos_ = 0;
            }
            return *this;
        }

        // Comparison operator with nullptr
        bool operator!=(std::nullptr_t) const
        {
            return remaining_;
        }

    private:
        const Chunk* chunk_;
        size_t pos_;
        size_t remaining_;
    };

    Iterator begin() const
    {
        return Iterator(this);
    }

    std::nullptr_t end() const
    {
        return nullptr;  // Simple sentinel indicating the end of iteration
    }

private:
    Chunk* last_;
    Chunk* first_;
    size_t totalCount_;
    Arena& arena_;
};

} // namespace clarisma
