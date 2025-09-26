// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <iterator>

namespace clarisma {

// TODO: review

/// @brief A vector-like container with inline storage for the first N elements.
///
/// @tparam T The type of elements stored in the vector.
/// @tparam N The number of elements to store on the stack before switching to heap.
///
/// @details
/// SmallVector behaves like `std::vector<T>`, but optimizes for small sizes by
/// storing the first N elements directly within the object. This can reduce
/// heap allocations and improve performance and cache locality for small arrays.
///
/// The container automatically switches to heap storage if the number of elements
/// exceeds N. It supports most of the `std::vector` interface, including:
/// - `push_back`, `pop_back`, `emplace_back`
/// - `operator[]`, `at()`, `front()`, `back()`
/// - Iterators and reverse iterators
/// - `reserve()`, `shrink_to_fit()`, and `clear()`
///
/// @note N must be greater than 0.
/// @note When moved, inline storage is preserved when possible.
///
template<typename T, std::size_t N>
class SmallVector
{
    static_assert(N > 0, "SmallVector capacity N must be > 0");

public:
    using value_type             = T;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = T&;
    using const_reference        = const T&;
    using pointer                = T*;
    using const_pointer          = const T*;
    using iterator               = T*;
    using const_iterator         = const T*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    SmallVector() noexcept
        : size_(0)
        , capacity_(N)
        , data_(inlineData_)
    {
    }

    SmallVector(const SmallVector& other)
        : size_(other.size_)
        , capacity_(other.capacity_ > N ? other.capacity_ : N)
        , data_(capacity_ > N ? allocate(capacity_) : inlineData_)
    {
        for (size_type i = 0; i < size_; ++i)
        {
            new (data_ + i) T(other.data_[i]);
        }
    }

    SmallVector(SmallVector&& other) noexcept
        : size_(other.size_)
        , capacity_(other.capacity_)
        , data_(other.data_)
    {
        if (other.data_ == other.inlineData_)
        {
            for (size_type i = 0; i < size_; ++i)
            {
                new (inlineData_ + i)
                    T(std::move(other.inlineData_[i]));
                other.inlineData_[i].~T();
            }
            data_     = inlineData_;
            capacity_ = N;
        }
        other.size_     = 0;
        other.capacity_ = N;
        other.data_     = other.inlineData_;
    }

    ~SmallVector()
    {
        clear();
        if (data_ != inlineData_)
        {
            ::operator delete(data_);
        }
    }

    SmallVector& operator=(const SmallVector& other)
    {
        if (this != &other)
        {
            clear();
            if (data_ != inlineData_)
            {
                ::operator delete(data_);
            }

            size_     = other.size_;
            capacity_ = other.capacity_ > N
                ? other.capacity_ : N;
            data_     = capacity_ > N
                ? allocate(capacity_) : inlineData_;

            for (size_type i = 0; i < size_; ++i)
            {
                new (data_ + i) T(other.data_[i]);
            }
        }
        return *this;
    }

    SmallVector& operator=(SmallVector&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            if (data_ != inlineData_)
            {
                ::operator delete(data_);
            }

            size_     = other.size_;
            capacity_ = other.capacity_;
            data_     = other.data_;

            if (other.data_ == other.inlineData_)
            {
                for (size_type i = 0; i < size_; ++i)
                {
                    new (inlineData_ + i)
                        T(std::move(other.inlineData_[i]));
                    other.inlineData_[i].~T();
                }
                data_     = inlineData_;
                capacity_ = N;
            }
            other.size_     = 0;
            other.capacity_ = N;
            other.data_     = other.inlineData_;
        }
        return *this;
    }

    // Element access
    reference operator[](size_type idx) noexcept
    {
        return data_[idx];
    }

    const_reference operator[](size_type idx) const noexcept
    {
        return data_[idx];
    }

    reference at(size_type idx)
    {
        if (idx >= size_)
        {
            throw std::out_of_range("SmallVector::at");
        }
        return data_[idx];
    }

    const_reference at(size_type idx) const
    {
        if (idx >= size_)
        {
            throw std::out_of_range("SmallVector::at");
        }
        return data_[idx];
    }

    reference front() noexcept { return data_[0]; }
    const_reference front() const noexcept { return data_[0]; }
    reference back() noexcept { return data_[size_ - 1]; }
    const_reference back() const noexcept
    {
        return data_[size_ - 1];
    }

    pointer data() noexcept { return data_; }
    const_pointer data() const noexcept { return data_; }

    // Iterators
    iterator begin() noexcept { return data_; }
    iterator end() noexcept { return data_ + size_; }
    const_iterator begin() const noexcept { return data_; }
    const_iterator end() const noexcept { return data_ + size_; }
    const_iterator cbegin() const noexcept { return data_; }
    const_iterator cend() const noexcept { return data_ + size_; }
    reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }
    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }
    const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    // Capacity
    bool empty() const noexcept { return size_ == 0; }
    size_type size() const noexcept { return size_; }
    size_type capacity() const noexcept { return capacity_; }

    void reserve(size_type newCap)
    {
        if (newCap <= capacity_)
        {
            return;
        }
        pointer newData = allocate(newCap);
        for (size_type i = 0; i < size_; ++i)
        {
            new (newData + i) T(std::move(data_[i]));
            data_[i].~T();
        }
        if (data_ != inlineData_)
        {
            ::operator delete(data_);
        }
        data_     = newData;
        capacity_ = newCap;
    }

    void shrink_to_fit()
    {
        if (size_ <= N && data_ != inlineData_)
        {
            for (size_type i = 0; i < size_; ++i)
            {
                new (inlineData_ + i)
                    T(std::move(data_[i]));
                data_[i].~T();
            }
            ::operator delete(data_);
            data_     = inlineData_;
            capacity_ = N;
        }
        else if (size_ < capacity_)
        {
            reserve(size_);
        }
    }

    // Modifiers
    void clear() noexcept
    {
        for (size_type i = 0; i < size_; ++i)
        {
            data_[i].~T();
        }
        size_ = 0;
    }

    void push_back(const T& value)
    {
        if (size_ < capacity_)
        {
            new (data_ + size_) T(value);
        }
        else
        {
            growAndEmplace(value);
        }
        ++size_;
    }

    void push_back(T&& value)
    {
        if (size_ < capacity_)
        {
            new (data_ + size_) T(std::move(value));
        }
        else
        {
            growAndEmplace(std::move(value));
        }
        ++size_;
    }

    template<typename... Args>
    reference emplace_back(Args&&... args)
    {
        if (size_ >= capacity_)
        {
            growAndReserve();
        }
        new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
        return back();
    }

    void pop_back() noexcept
    {
        if (size_ > 0)
        {
            --size_;
            data_[size_].~T();
        }
    }

    void swap(SmallVector& other) noexcept
    {
        using std::swap;
        if (data_ == inlineData_ && other.data_ == other.inlineData_)
        {
            SmallVector temp(std::move(*this));
            *this = std::move(other);
            other = std::move(temp);
        }
        else
        {
            swap(size_,     other.size_);
            swap(capacity_, other.capacity_);
            swap(data_,     other.data_);
        }
    }

protected:
    // no protected members

private:
    void growAndReserve()
    {
        size_type newCap =
            capacity_ ? (capacity_ * 2) : 1;
        reserve(newCap);
    }

    template<typename U>
    void growAndEmplace(U&& value)
    {
        growAndReserve();
        new (data_ + size_) T(std::forward<U>(value));
    }

    pointer allocate(size_type count)
    {
        return static_cast<pointer>(
            ::operator new(sizeof(T) * count)
        );
    }

    size_type size_;
    size_type capacity_;
    T*          data_;
    alignas(T) T inlineData_[N];
};

} // namespace clarisma
