//
// Created by marti on 9/29/2024.
//

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

inline char* integerNice(char* p, int64_t d)
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

inline char* doubleReverse(char** pEnd, double d, int precision, bool zeroFill)
{
    assert(precision >= 0 && precision <= 15);
    char* end = *pEnd;
    double multiplier = Math::POWERS_OF_10[precision];
    long long roundedScaled = static_cast<long long>(round(d * multiplier));
    long long intPart = static_cast<long long>(roundedScaled / multiplier);
    unsigned long long fracPart = static_cast<unsigned long long>(
        abs(roundedScaled - intPart * multiplier));
    char* start = fractionalReverse(fracPart, &end, precision, zeroFill);
    if (start != end) *(--start) = '.';
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

} // namespace clarisma