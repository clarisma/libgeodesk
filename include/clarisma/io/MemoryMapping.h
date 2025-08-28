// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <cstdint>
#include <span>

#include "FileHandle.h"

using std::byte;

namespace clarisma {

/// @brief A memory-mapped portion of a file. Unmaps automatically
/// upon destruction.
///
class MemoryMapping
{
public:
    MemoryMapping() {}

    MemoryMapping(byte* data, uint64_t size) :
        data_(data),
        size_(size) {}

    MemoryMapping(FileHandle file, uint64_t ofs, size_t size) :
        data_(file.map(ofs, size)),
        size_(size)
    {
    }

    MemoryMapping(MemoryMapping&& other) noexcept :
        data_(std::exchange(other.data_, nullptr)),
        size_(std::exchange(other.size_, 0))
    {
    }

    MemoryMapping& operator=(MemoryMapping&& other) noexcept
    {
        if (this != &other)
        {
            if (data_) FileHandle::unmap(data_, size_);
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    MemoryMapping(const MemoryMapping&) = delete;
    MemoryMapping& operator=(const MemoryMapping&) = delete;

    ~MemoryMapping()
    {
        if (data_) FileHandle::unmap(data_, size_);
    }

    byte* data() const { return data_; }
    uint64_t size() const { return size_; }

    operator std::span<std::byte>() noexcept
    {
        return { data_,size_ };
    }

    operator std::span<const std::byte>() const noexcept
    {
        return { data_, size_ };
    }

private:
    byte* data_ = nullptr;
    size_t size_ = 0;
};

} // namespace clarisma

