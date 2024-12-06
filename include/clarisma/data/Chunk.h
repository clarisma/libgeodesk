// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

namespace clarisma {

template<typename T>
class Chunk 
{
public:
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    static Chunk* create(size_t size)
    {
        void* memory = std::malloc(sizeof(Header) + size);
        if (!memory) throw std::bad_alloc();
        return new (memory) Chunk(size);
    }

    static void destroy(Chunk* chunk)
    {
        std::free(chunk);
    }

    T* data() { return data_; }
    const T* data() const { return data_; }
    size_t size() const { return header_.size; }
    Chunk* next() const { return header_.next; }
    void setNext(Chunk* next) { header_.next = next; }
    void trim(size_t size)
    {
        assert(size <= header_.size);
        header_.size = size;
    }

    static Chunk<T>* ptrFromData(T* p)
    {
        uint8_t* raw = reinterpret_cast<uint8_t*>(p) -
            offsetof(Chunk<T>, data_);
        return reinterpret_cast<Chunk<T>*>(raw);
    }

private:
    struct Header
    {
        Chunk* next;
        size_t size;
    };

    Chunk(size_t size) : header_(nullptr,size) {}

    Header header_;
    T data_[1];
};


template<typename T>
class ChunkChain
{
public:
    ChunkChain() noexcept : first_(nullptr) {}
    explicit ChunkChain(size_t size) noexcept :
        first_(Chunk<T>::create(size)) {}
    ChunkChain(ChunkChain&& other) noexcept :
        first_(other.first_)
    {
        other.first_ = nullptr;
    }

    ChunkChain& operator=(ChunkChain&& other) noexcept
    {
        if (this != &other)
        {
            release();
            first_ = other.first_;
            other.first_ = nullptr;
        }
        return *this;
    }

    ChunkChain(const ChunkChain&) = delete;
    ChunkChain& operator=(const ChunkChain&) = delete;

    bool isEmpty() const noexcept { return first_ == nullptr; }
    Chunk<T>* first() const { return first_; }

private:
    void release()
    {
        while (first_)
        {
            Chunk<T>* next = first_->next();
            Chunk<T>::destroy(first_);
            first_ = next;
        }
    }

    Chunk<T>* first_;
};

} // namespace clarisma
