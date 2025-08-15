// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <cstdint>
#include <span>

#include "FileHandle.h"

using std::byte;

namespace clarisma {

class MemoryMapping
{
public:
    MemoryMapping(byte* data, uint64_t size) :
        data_(data),
        size_(size) {}

    ~MemoryMapping()
    {
        if (data_) FileHandle::unmap(data_, size_);
    }

    byte* data() const { return data_; }
    uint64_t size() const { return size_; }

    operator std::span<std::byte>() noexcept
    {
        return { data_, static_cast<size_t>(size_) };
    }

    operator std::span<const std::byte>() const noexcept
    {
        return { data_, static_cast<size_t>(size_) };
    }

private:
    byte* data_ = nullptr;
    uint64_t size_ = 0;
};

} // namespace clarisma

