// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/math/Decimal.h>
#include <limits>

namespace clarisma {

int64_t Decimal::parse(std::string_view s, bool strict)
{
    int64_t value = 0;
    int scale = 0;
    bool seenZero = false;
    bool seenNonZero = false;
    bool leadingZeroes = false;
    bool trailingNonNumeric = false;
    bool seenDot = false;
    bool negative = false;
    const char* p = s.data();
    const char* end = s.data() + s.size();

    if (p == end)	[[unlikely]]
    {
        return INVALID;
    }
    if (*p == '-')
    {
        negative = true;
        p++;
        if (p == end)	[[unlikely]]
        {
            return INVALID;
        }
    }
    else if (*p == '+')
    {
        if (strict) return INVALID;
        p++;
        if (p == end)	[[unlikely]]
        {
            return INVALID;
        }
    }

    while(p < end)
    {
        char ch = *p++;
        if (ch == '0')
        {
            leadingZeroes = seenZero & !seenNonZero;
            seenZero = true;
            value *= 10;
            if ((value & 0xf800'0000'0000'0000ULL) != 0) return INVALID;
            continue;
        }
        if (ch == '.')
        {
            seenDot = true;
            while (p < end)
            {
                ch = *p++;
                if (ch < '0' || ch > '9')
                {
                    trailingNonNumeric = true;
                    break;
                }
                value = value * 10 + (ch - '0');
                if ((value & 0xf800'0000'0000'0000ULL) != 0) return INVALID;
                scale++;
            }
            break;
        }
        if (ch < '0' || ch > '9')
        {
            trailingNonNumeric = true;
            break;
        }
        leadingZeroes = seenZero & !seenNonZero;
        seenNonZero = true;
        value = value * 10 + (ch - '0');
        if ((value & 0xf800'0000'0000'0000ULL) != 0) return INVALID;
    }
    if (strict)
    {
        if (trailingNonNumeric) return INVALID;
        if (seenDot & (scale == 0 | (!seenZero & !seenNonZero)))
        {
            return INVALID;
        }
        if (leadingZeroes) return INVALID;
        if (value == 0 && negative) return INVALID;
    }
    if (scale > 15) return INVALID;
    return ((negative ? -value : value) << 4) | scale;
}

char* Decimal::format(char* buf) const noexcept
{
    if (value_ == INVALID)  [[unlikely]]
    {
        memcpy(buf, "invalid", 8);
        return buf + 7;
    }

    char temp[32];
    char* end = temp + sizeof(temp);
    char* start = Format::integerReverse(mantissa(), end);
    size_t len = end - start;
    int scale = this->scale();
    if (scale == 0)
    {
        memcpy(buf, start, len);
    }
    else
    {
        size_t wholePartLen = len - scale;
        if (wholePartLen == 0)
        {
            // Insert leading '0' before decimal point
            buf[0] = '0';
            buf[1] = '.';
            memcpy(buf + 2, start, len);
            len += 2;
        }
        else
        {
            memcpy(buf, start, wholePartLen);
            buf[wholePartLen] = '.';
            memcpy(buf + wholePartLen + 1, start + wholePartLen, len - wholePartLen);
            len++;
        }
    }
    buf[len] = 0;
    return buf + len;
}

} // namespace clarisma
