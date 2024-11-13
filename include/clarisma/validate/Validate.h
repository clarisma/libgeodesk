// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <clarisma/text/Format.h>

namespace clarisma {

class ValueException : public std::runtime_error
{
public:
    explicit ValueException(const char* message)
        : std::runtime_error(message) {}

    explicit ValueException(const std::string& message)
        : std::runtime_error(message) {}
};


class Validate
{
public:
    static int64_t max(int64_t value, int64_t maxValue)
    {
        if (value > maxValue)
        {
            throw ValueException(Format::format("Exceeds maximum value (%d)", maxValue));
        }
        return value;
    }

    static int maxInt(int64_t value, int maxValue)
    {
        return static_cast<int>(max(value, maxValue));
    }

    static int64_t longValue(const char* s)
    {
        char* pEnd;
        int64_t v = std::strtoll(s, &pEnd, 10);
        if (pEnd == s)
        {
            if(*s == 0) throw ValueException(Format::format("Expected number", s));
            throw ValueException(Format::format("Expected number instead of %s", s));
        }
        return v;
    }

    static int intValue(const char* s)
    {
        return static_cast<int>(longValue(s));
    }

    static int64_t longValue(int64_t v, int64_t min, int64_t max)
    {
        if (v < min || v > max)
        {
            throw ValueException(Format::format("Must be %lld to %lld", min, max));
        }
        return v;
    }

    static int64_t longValue(const char* s, int64_t min, int64_t max)
    {
        return longValue(longValue(s), min, max);
    }

    static int intValue(int v, int min, int max)
    {
        return static_cast<int>(longValue(v, min, max));
    }

    static int intValue(const char* s, int32_t min, int32_t max)
    {
        return intValue(intValue(s), min, max);
    }

    static double doubleValue(double v, double min, double max)
    {
        if (v < min || v > max)
        {
            throw ValueException(Format::format("Must be %f to %f", min, max));
        }
        return v;
    }
};

} // namespace clarisma
