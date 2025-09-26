// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <memory>
#include <span>
#include <utility>

namespace clarisma {

template <typename T>
class Block
{
public:
    using value_type = T;

    // Default constructor creates an empty block
    Block() :
        data_(nullptr),
        size_(0)
    {
    }

    // Constructor that allocates a Block of the given size
    explicit Block(size_t size) :
        data_(size ? std::make_unique<T[]>(size) : nullptr),
        size_(size)
    {
    }

    // Constructor that takes ownership of an existing heap-allocated array
    Block(T* data, size_t size) :
        data_(data),
        size_(size)
    {
    }

    // Constructor that takes ownership from an existing std::unique_ptr<T[]>
    Block(std::unique_ptr<T[]> data, size_t size) :
        data_(std::move(data)),
        size_(size)
    {
    }

    // Move constructor
    Block(Block&& other) noexcept :
        data_(std::move(other.data_)),
        size_(other.size_)
    {
        other.size_ = 0;
    }

    // Move assignment operator
    Block& operator=(Block&& other) noexcept
    {
        if (this != &other)
        {
            data_ = std::move(other.data_);
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    // Disable copying
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    // TODO: This is unintuitive, since in most cases we want the size as well,
    //  which will now be invalid
    std::unique_ptr<T[]> take()
    {
        size_ = 0;
        return std::move(data_);
    }

    // Accessor for size
    size_t size() const noexcept { return size_; }

    // Accessor for data
    const T* data() const noexcept { return data_.get(); }
    T* data() noexcept { return data_.get(); }

    // TODO: dupe of take()
    std::unique_ptr<T[]> takeData()
    {
        size_ = 0;
        return std::move(data_);
    }

    // Access elements
    T& operator[](size_t index) const
    {
        assert(index < size_);
        return data_[index];
    }

    operator std::span<const T>() const noexcept    // NOLINT implicit on purpose
    {
        return { data_.get(), size_ };
    }

private:
    std::unique_ptr<T[]> data_;
    size_t size_;
};


using ByteBlock = Block<uint8_t>;
using CharBlock = Block<char>;

} // namespace clarisma
