// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace clarisma {

/// \brief A vector-like container that stores up to S elements inline
/// (without heap allocation), and only allocates once the element
/// count exceeds S.
///
template<typename T, size_t S>
class SmallVector
{
public:
    using SizeType = uint32_t;      // customizable later

    using value_type = T;
    using size_type = SizeType;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;

    static_assert(S > 0, "SmallVector requires at least one inline slot");
    static_assert(S <= std::numeric_limits<SizeType>::max(),
        "Inline capacity exceeds SizeType range");

    SmallVector() noexcept :
        data_(inlinePtr()),
        size_(0),
        capacity_(static_cast<SizeType>(S))
    {
    }

    explicit SmallVector(SizeType size) : SmallVector()
    {
        resize(size);
    }

    SmallVector(SizeType size, const T& value) : SmallVector()
    {
        resize(size, value);
    }

    SmallVector(std::initializer_list<T> init) : SmallVector()
    {
        reserve(static_cast<SizeType>(init.size()));
        for(const T& v : init)
        {
            new (data_ + size_) T(v);
            ++size_;
        }
    }

    SmallVector(const SmallVector& other) : SmallVector()
    {
        reserve(other.size_);
        std::uninitialized_copy_n(other.data_, other.size_, data_);
        size_ = other.size_;
    }

    SmallVector(SmallVector&& other)
        noexcept(std::is_nothrow_move_constructible_v<T>) :
        SmallVector()
    {
        adoptContentsOf(other);
    }

    ~SmallVector() noexcept
    {
        std::destroy_n(data_, size_);
        freeHeap();
    }

    SmallVector& operator=(const SmallVector& other)
    {
        if(this != &other)
        {
            clear();
            reserve(other.size_);
            std::uninitialized_copy_n(other.data_, other.size_, data_);
            size_ = other.size_;
        }
        return *this;
    }

    SmallVector& operator=(SmallVector&& other)
        noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if(this != &other)
        {
            std::destroy_n(data_, size_);
            freeHeap();
            data_ = inlinePtr();
            size_ = 0;
            capacity_ = static_cast<SizeType>(S);
            adoptContentsOf(other);
        }
        return *this;
    }

    // --- Element access ---------------------------------------------------

    T& operator[](SizeType i) noexcept
    {
        return data_[i];
    }

    const T& operator[](SizeType i) const noexcept
    {
        return data_[i];
    }

    T& at(SizeType i)
    {
        if(i >= size_) throw std::out_of_range("SmallVector::at");
        return data_[i];
    }

    const T& at(SizeType i) const
    {
        if(i >= size_) throw std::out_of_range("SmallVector::at");
        return data_[i];
    }

    T& front() noexcept { return data_[0]; }
    const T& front() const noexcept { return data_[0]; }
    T& back() noexcept { return data_[size_ - 1]; }
    const T& back() const noexcept { return data_[size_ - 1]; }

    T* data() noexcept { return data_; }
    const T* data() const noexcept { return data_; }

    // --- Iterators ----------------------------------------------------------

    iterator begin() noexcept { return data_; }
    const_iterator begin() const noexcept { return data_; }
    const_iterator cbegin() const noexcept { return data_; }
    iterator end() noexcept { return data_ + size_; }
    const_iterator end() const noexcept { return data_ + size_; }
    const_iterator cend() const noexcept { return data_ + size_; }

    // --- Capacity -----------------------------------------------------------

    bool empty() const noexcept { return size_ == 0; }
    SizeType size() const noexcept { return size_; }
    SizeType capacity() const noexcept { return capacity_; }

    static constexpr SizeType max_size() noexcept
    {
        return std::numeric_limits<SizeType>::max();
    }

    bool isInline() const noexcept
    {
        return data_ == inlinePtr();
    }

    void reserve(SizeType minCapacity)
    {
        if(minCapacity > capacity_)    [[unlikely]]
        {
            reallocate(minCapacity);
        }
    }

    // --- Modifiers ----------------------------------------------------------

    void clear() noexcept
    {
        std::destroy_n(data_, size_);
        size_ = 0;
    }

    void push_back(const T& value)
    {
        emplace_back(value);
    }

    void push_back(T&& value)
    {
        emplace_back(std::move(value));
    }

    template<typename... Args>
    T& emplace_back(Args&&... args)
    {
        if(size_ == capacity_)    [[unlikely]]
        {
            reallocate(nextCapacity(size_ + 1));
        }
        T* p = new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
        return *p;
    }

    void pop_back() noexcept
    {
        --size_;
        data_[size_].~T();
    }

    void resize(SizeType newSize)
    {
        if(newSize > size_)
        {
            reserve(newSize);
            std::uninitialized_value_construct_n(
                data_ + size_, newSize - size_);
        }
        else
        {
            std::destroy_n(data_ + newSize, size_ - newSize);
        }
        size_ = newSize;
    }

    void resize(SizeType newSize, const T& value)
    {
        if(newSize > size_)
        {
            reserve(newSize);
            std::uninitialized_fill_n(data_ + size_, newSize - size_, value);
        }
        else
        {
            std::destroy_n(data_ + newSize, size_ - newSize);
        }
        size_ = newSize;
    }

private:
    T* inlinePtr() noexcept
    {
        return reinterpret_cast<T*>(inlineData_);
    }

    const T* inlinePtr() const noexcept
    {
        return reinterpret_cast<const T*>(inlineData_);
    }

    static T* allocate(SizeType capacity)
    {
        return static_cast<T*>(operator new(
            static_cast<size_t>(capacity) * sizeof(T),
            std::align_val_t(alignof(T))));
    }

    void freeHeap() noexcept
    {
        if(!isInline())    [[unlikely]]
        {
            ::operator delete(data_, std::align_val_t(alignof(T)));
        }
    }

    SizeType nextCapacity(SizeType minCapacity) const
    {
        if(minCapacity < capacity_)    [[unlikely]]   // SizeType overflow
        {
            throw std::length_error("SmallVector: capacity overflow");
        }
        SizeType doubled = (capacity_ > max_size() - capacity_) ?
            max_size() : (capacity_ * 2);
        return (doubled > minCapacity) ? doubled : minCapacity;
    }

    /// Moves all elements into a new heap buffer of the given capacity.
    /// Provides the strong exception guarantee (falls back to copying
    /// if T's move constructor may throw, like std::vector).
    ///
    void reallocate(SizeType newCapacity)
    {
        T* newData = allocate(newCapacity);
        if constexpr(std::is_nothrow_move_constructible_v<T> ||
            !std::is_copy_constructible_v<T>)
        {
            std::uninitialized_move_n(data_, size_, newData);
        }
        else
        {
            try
            {
                std::uninitialized_copy_n(data_, size_, newData);
            }
            catch(...)
            {
                ::operator delete(newData, std::align_val_t(alignof(T)));
                throw;
            }
        }
        std::destroy_n(data_, size_);
        freeHeap();
        data_ = newData;
        capacity_ = newCapacity;
    }

    /// Takes over the contents of another SmallVector, which is left
    /// empty. Assumes this SmallVector is empty and inline.
    ///
    void adoptContentsOf(SmallVector& other) noexcept(
        std::is_nothrow_move_constructible_v<T>)
    {
        if(!other.isInline())
        {
            // Heap contents: just steal the buffer
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = other.inlinePtr();
            other.size_ = 0;
            other.capacity_ = static_cast<SizeType>(S);
        }
        else
        {
            // Inline contents: must move element by element
            std::uninitialized_move_n(other.data_, other.size_, data_);
            size_ = other.size_;
            other.clear();
        }
    }

    T* data_;
    SizeType size_;
    SizeType capacity_;
    alignas(alignof(T)) std::byte inlineData_[S * sizeof(T)];
};

} // namespace clarisma
