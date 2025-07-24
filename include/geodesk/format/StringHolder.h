// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>
#include <clarisma/math/Decimal.h>
#include <clarisma/util/ShortVarString.h>

namespace geodesk {

class StringHolder 
{
public:
    StringHolder()
    {
        data_.inlined.flaggedLen = 0;
    }

    explicit StringHolder(const clarisma::ShortVarString* s)
    {
        data_.referenced.flag = REFERENCED_FLAG;
        data_.referenced.len = s->length();
        data_.referenced.ptr = s->data();
    }

    // TODO: The inline space cannot accommodate the full range of Decimla values;
    //  it is sufficient for tag values stored as numbers
    explicit StringHolder(clarisma::Decimal d)
    {
        const char* end = d.format(data_.inlined.data);
        data_.inlined.flaggedLen = end - data_.inlined.data;
        assert(data_.inlined.flaggedLen <= 14);
        // Decimal::format() appends a 0-char
    }

    const char* data() const noexcept
    {
        return isInlined() ? data_.inlined.data : data_.referenced.ptr;
    }

    size_t size() const noexcept
    {
        return isInlined() ? data_.inlined.flaggedLen :
            data_.referenced.len;
    }

    std::string_view toStringView() const noexcept
    {
        if (isInlined())
        {
            return {data_.inlined.data, data_.inlined.flaggedLen};
        }
        return {data_.referenced.ptr, data_.referenced.len};
    }

    template<typename Stream>
    void writeTo(Stream& out) const
    {
        if (isInlined())
        {
            out.write(data_.inlined.data, data_.inlined.flaggedLen);
        }
        else
        {
            out.write(data_.referenced.ptr, data_.referenced.len);
        }
    }

private:
    bool isInlined() const
    {
        return (data_.inlined.flaggedLen & REFERENCED_FLAG) == 0;
    }

    union
    {
        struct
        {
            unsigned char flaggedLen;
            char data[15];
        }
        inlined;

        struct
        {
            unsigned char flag;
            // The compiler will typically insert 3 bytes of padding here
            // so that 'len' is aligned on a 4-byte boundary.
            uint32_t len;
            const char *ptr;
        }
        referenced;
    }
    data_;

    static constexpr unsigned char REFERENCED_FLAG = 128;
};

static_assert(sizeof(StringHolder) == 16, "StringHolder must be 16 bytes");

template<typename Stream>
Stream& operator<<(Stream& out, const StringHolder& v)
{
    v.writeTo(out);
    return out;
}

} // namespace geodesk
