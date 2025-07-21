// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <iterator>

namespace clarisma {

template<typename T, size_t N>
class SmallArray
{
public:
    explicit SmallArray(size_t size) noexcept
    {
        if(size <= N)  [[likely]]
        {
            data_ = reinterpret_cast<T*>(inlineData_);
        }
        else
        {
            data_ = static_cast<T*>(operator new[](
                size * sizeof(T), std::align_val_t(alignof(T))));
        }
    }

    SmallArray(const SmallArray& other) = delete;
    SmallArray& operator=(const SmallArray&) = delete;

    ~SmallArray() noexcept
    {
        if(!isInline())    [[unlikely]]
        {
            ::operator delete(data_);
        }
    }

    T& operator[](size_t i) noexcept
    {
        return data_[i];
    }

    const T& operator[](size_t i) const noexcept
    {
        return data_[i];
    }


private:
    bool isInline() const noexcept
    {
        return reinterpret_cast<std::byte*>(data_) == inlineData_;
    }

    T* data_;
    std::aligned_storage_t<sizeof(T), alignof(T)> inlineData_[N];
};

} // namespace clarisma
