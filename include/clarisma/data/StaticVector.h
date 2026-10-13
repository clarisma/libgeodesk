// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only


#pragma once

#include <cassert>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace clarisma {

/// \brief Fixed-capacity vector with inline storage.
/// Elements are constructed on demand.
///
template<typename T, std::size_t CAPACITY>
class StaticVector
{
public:
    /// Forward iterator over constructed elements.
    template<bool IS_CONST>
    class Iterator
    {
        template<bool> friend class Iterator;

    public:
        using Container = std::conditional_t<IS_CONST,
            const StaticVector, StaticVector>;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = std::conditional_t<IS_CONST, const T&, T&>;
        using pointer = std::conditional_t<IS_CONST, const T*, T*>;
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept = std::forward_iterator_tag;

        Iterator() noexcept = default;

        /// Convert a mutable iterator to a const iterator.
        template<bool B = IS_CONST> requires B
        Iterator(const Iterator<false>& other) noexcept :
            owner_(other.owner_),
            index_(other.index_)
        {
        }

        /// Construct an iterator at the specified position.
        Iterator(Container* owner, std::size_t index) noexcept :
            owner_(owner),
            index_(index)
        {
        }

        /// Access the current element.
        reference operator*() const noexcept { return (*owner_)[index_]; }

        /// Access the current element by pointer.
        pointer operator->() const noexcept { return &**this; }

        /// Advance to the next element.
        Iterator& operator++() noexcept
        {
            ++index_;
            return *this;
        }

        /// Advance and return the previous position.
        Iterator operator++(int) noexcept
        {
            auto previous = *this;
            ++*this;
            return previous;
        }

        /// Compare iterator positions.
        bool operator==(const Iterator&) const noexcept = default;

    private:
        Container* owner_ = nullptr;
        std::size_t index_ = 0;
    };

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    StaticVector() noexcept {};

    StaticVector(const StaticVector&) = delete;
    StaticVector& operator=(const StaticVector&) = delete;
    StaticVector(StaticVector&&) = delete;
    StaticVector& operator=(StaticVector&&) = delete;

    /// Destroy all constructed elements.
    ~StaticVector() noexcept { clear(); }

    /// Construct an element at the end.
    /// Terminates if capacity is exceeded.
    template<typename... Args>
    T& emplace_back(Args&&... args)
    {
        assert(!full());
        if (full()) std::terminate();

        void* slot = storage_ + size_ * sizeof(T);
        T* element = ::new (slot) T(std::forward<Args>(args)...);

        ++size_;
        return *element;
    }

    /// Construct an element if space is available.
    /// Return nullptr if the vector is full.
    template<typename... Args>
    T* try_emplace_back(Args&&... args)
    {
        if (full()) return nullptr;
        return &emplace_back(std::forward<Args>(args)...);
    }

    /// Destroy the last element.
    /// Requires a nonempty vector.
    void pop_back() noexcept
    {
        assert(!empty());
        // if (empty()) std::terminate();

        std::destroy_at(elementAt(size_ - 1));
        --size_;
    }

    T& back() noexcept
    {
        assert(!empty());
        return *elementAt(size_ - 1);
    }

    /// Destroy all constructed elements in reverse order.
    void clear() noexcept
    {
        while (!empty()) pop_back();
    }

    /// Access an element by index.
    T& operator[](std::size_t index) noexcept
    {
        assert(index < size_);
        return *elementAt(index);
    }

    /// Access an element by index.
    const T& operator[](std::size_t index) const noexcept
    {
        assert(index < size_);
        return *elementAt(index);
    }

    /// Return the number of constructed elements.
    std::size_t size() const noexcept { return size_; }

    /// Return the maximum number of elements.
    constexpr std::size_t capacity() const noexcept
    {
        return CAPACITY;
    }

    /// Return whether the vector is empty.
    bool empty() const noexcept { return size_ == 0; }

    /// Return whether the vector is full.
    bool full() const noexcept { return size_ == CAPACITY; }

    /// Return an iterator to the first element.
    iterator begin() noexcept { return {this, 0}; }

    /// Return an iterator past the last element.
    iterator end() noexcept { return {this, size_}; }

    /// Return an iterator to the first element.
    const_iterator begin() const noexcept { return {this, 0}; }

    /// Return an iterator past the last element.
    const_iterator end() const noexcept { return {this, size_}; }

private:
    /// Return the address of a constructed element.
    T* elementAt(std::size_t index) noexcept
    {
        return std::launder(reinterpret_cast<T*>(
            storage_ + index * sizeof(T)));
    }

    /// Return the address of a constructed element.
    const T* elementAt(std::size_t index) const noexcept
    {
        return std::launder(reinterpret_cast<const T*>(
            storage_ + index * sizeof(T)));
    }

    std::size_t size_ = 0;

    alignas(T) std::byte storage_[
        sizeof(T) * (CAPACITY ? CAPACITY : 1)];
};

} // namespace clarisma
