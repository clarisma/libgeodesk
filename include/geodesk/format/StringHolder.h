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

    // TODO: The inline space cannot accommodate the full range of Decimal values;
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
        return isInlined() ? (data_.inlined.flaggedLen & ~REFERENCED_FLAG) :
            data_.referenced.len;
    }

private:
    bool isInlined() const { return data_.inlined.flaggedLen & REFERENCED_FLAG; }

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

} // namespace geodesk
