// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <cmath>
#include <clarisma/math/Math.h>
#include <clarisma/text/Format.h>

namespace clarisma::Format {

/*
char* formatDouble(char* buf, double d, int precision, bool zeroFill)
{
    assert(precision >= 0 && precision <= 15);
    double multiplier = Math::POWERS_OF_10[precision];
    long long roundedScaled = static_cast<long long>(round(d * multiplier));
    long long intPart = static_cast<long long>(roundedScaled / multiplier);
    unsigned long long fracPart = static_cast<unsigned long long>(
        abs(roundedScaled - intPart * multiplier));
    char* end = buf + sizeof(buf);
    char* start = formatFractionalReverse(fracPart, &end, precision, zeroFill);
    if (start != end) *(--start) = '.';
    start = formatLongReverse(intPart, start, d < 0);
    writeBytes(start, end - start);
}

static char* formatFractionalReverse(unsigned long long d, char** pEnd, int precision, bool zeroFill)
{
    char* end = *pEnd;
    char* start = end - precision;
    char* p = end;
    while(p > start)
    {
        lldiv_t result = lldiv(d, 10);
        if (p == end && result.rem == 0 && !zeroFill)
        {
            end--;		// skip trailing zeroes
            p--;
        }
        else
        {
            *(--p) = static_cast<char>('0' + result.rem);
        }
        d = result.quot;
    }
    *pEnd = end;
    return p;
}
*/

char* integerNice(char* p, int64_t d)
{
    char buf[32];
    char* end = buf + sizeof(buf);
    char* start = unsignedIntegerReverse(d, end);
    if(d < 0) *p++ ='-';
    int firstRun = static_cast<int>((end - start) % 3);
    if(firstRun == 0) firstRun = 3;
    memcpy(p, start, firstRun);
    p += firstRun;
    start += firstRun;
    while(start < end)
    {
        *p++ =',';
        *p++ = *start++;
        *p++ = *start++;
        *p++ = *start++;
    }
    return p;
}

inline char* fractionalReverse(unsigned long long d, char** pEnd, int precision, bool zeroFill)
{
    char* end = *pEnd;
    char* start = end - precision;
    char* p = end;
    while(p > start)
    {
        lldiv_t result = lldiv(d, 10);
        if (p == end && result.rem == 0 && !zeroFill)
        {
            end--;		// skip trailing zeroes
            p--;
        }
        else
        {
            *(--p) = static_cast<char>('0' + result.rem);
        }
        d = result.quot;
    }
    *pEnd = end;
    return p;
}

char* doubleReverse(char** pEnd, double d, int precision, bool zeroFill)
{
    assert(precision >= 0 && precision <= 15);
    double multiplier = Math::POWERS_OF_10[precision];
    long long roundedScaled = static_cast<long long>(round(d * multiplier));
    long long intPart = static_cast<long long>(roundedScaled / multiplier);
    unsigned long long fracPart = static_cast<unsigned long long>(
        abs(roundedScaled - intPart * multiplier));
    char* start = fractionalReverse(fracPart, pEnd, precision, zeroFill);
    if (start != *pEnd) *(--start) = '.';
    return integerReverse(intPart, start);
}

char* timeAgo(char* buf, int64_t secs)
{
    int64_t d;
    const char* unit;

    if (secs < 60)
    {
        d = secs;
        unit = "second";
    }
    else if (secs < 3600)
    {
        d = secs / 60;
        unit = "minute";
    }
    else if (secs < 86400)
    {
        d = secs / 3600;
        unit = "hour";
    }
    else if (secs < 172800)
    {
        strcpy(buf, "yesterday");
        return buf + 9;
    }
    else if (secs < 2592000)
    {
        d = secs / 86400;
        unit = "day";
    }
    else if (secs < 31536000)
    {
        d = secs / 2592000;
        unit = "month";
    }
    else
    {
        d = secs / 31536000;
        unit = "year";
    }
    char *p = integer(buf, d);
    *p++ = ' ';
    while(*unit)
    {
        *p++ = *unit++;
    }
    if(d != 1) *p++ = 's';
    strcpy(p, " ago");
    return p + 4;
}


static const double FILE_SIZE_INTERVALS[] =
{
    1,
    1024.0,                  // KB
    1024.0 * 1024,           // MB
    1024.0 * 1024 * 1024,    // GB
    1024.0 * 1024 * 1024 * 1024,       // TB
    1024.0 * 1024 * 1024 * 1024 * 1024, // PB
    1024.0 * 1024 * 1024 * 1024 * 1024 * 1024 // EB
};

static const char FILE_SIZE_UNITS[] = "\0KMGTPE";

// TODO: pre-rounding 12500000 bytes should be 12 MB

char* fileSizeNice(char* p, uint64_t size)
{
    double d = static_cast<double>(size);
    int i=1;
    for (; i<7; i++)
    {
        if (d < FILE_SIZE_INTERVALS[i]) break;
    }
    d /= FILE_SIZE_INTERVALS[i-1];
    double rounded = std::floor(d * 10.0 + 0.5) / 10.0;
    double whole;
    if (rounded >= 10.0)
    {
        whole = std::floor(rounded + 0.5);
    }
    else
    {
        whole = std::floor(rounded);
    }

    p = integer(p, static_cast<int64_t>(whole));
    if (d < 10.0 && rounded != whole)
    {
        *p++ = '.';
        *p++ = '0' + static_cast<char>((rounded - whole) * 10.0);
    }
    *p++ = ' ';
    char unit = FILE_SIZE_UNITS[i-1];
    if (unit) *p++ = unit;
    *p++ = 'B';
    *p = '\0';
    return p;
}

// 0-terminates
inline char* formatFractional(char* buf, unsigned long long d, int precision, bool zeroFill)
{
    char* start = buf + 1;
    char* end = start + precision;
    char* p = end;
    while(p > start)
    {
        lldiv_t result = lldiv(d, 10);
        if (p == end && result.rem == 0 && !zeroFill)
        {
            end--;		// skip trailing zeroes
            p--;
        }
        else
        {
            *(--p) = static_cast<char>('0' + result.rem);
        }
        d = result.quot;
    }
    buf[0] = '.';
    end -= (start == end);
    *end = '\0';
    return end;
}


char* formatDouble(char* out, double d, int precision, bool zeroFill)
{
    assert(precision >= 0 && precision <= 15);
    char buf[64];
    double multiplier = Math::POWERS_OF_10[precision];
    long long roundedScaled = static_cast<long long>(std::abs(round(d * multiplier)));
    long long intPart = static_cast<long long>(roundedScaled / multiplier);
    unsigned long long fracPart = static_cast<unsigned long long>(
        roundedScaled - intPart * multiplier);

    // Format the whole portion
    char* end = buf + sizeof(buf);
    char *start = unsignedIntegerReverse(intPart, end);
    // can't use signed because it drops sign if int portion is 0
    *(start - 1) = '-';
    start = start - static_cast<int>(d < 0);
    auto wholePartLen = end - start;
    memcpy(out, start, wholePartLen);
    start = out + wholePartLen;

    // Format the fractional portion
    return formatFractional(start, fracPart, precision, zeroFill);
}


} // namespace clarisma::Format