// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdint>
#include <clarisma/math/Decimal.h>
#include <clarisma/util/ShortVarString.h>
#include <clarisma/util/streamable.h> // for << operator support

namespace geodesk {

using clarisma::operator<<;

class StringHolder 
{
public:
    static constexpr int MAX_INLINED_LENGTH = 15;

    StringHolder()
    {
        data_.inlined.flaggedLen = 0;
    }

    static StringHolder inlineCopy(const char *s, size_t size)
    {
        StringHolder holder;
        unsigned char len = static_cast<unsigned char>(
            std::min(size, std::size_t(15)));
        holder.data_.inlined.flaggedLen = len;
        memcpy(holder.data_.inlined.data, s, len);
        return holder;
    }

    static StringHolder inlineCopy(std::string_view s)
    {
        return inlineCopy(s.data(), s.size());
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

    /*
    char* inlineDataMax15()
    {
        assert(isInlined());
        return data_.inlined.data;
    }
    */

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
    void format(Stream& out) const
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

} // namespace geodesk
