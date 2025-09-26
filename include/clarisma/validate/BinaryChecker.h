// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/validate/Checker.h>
#include <clarisma/validate/Validate.h>
#include <clarisma/util/varint_safe.h>

namespace clarisma {

class ShortVarString;

class BinaryChecker : public Checker
{
public:
    BinaryChecker(const uint8_t* data, size_t size) :
        p_(data),
        mark_(data),
        start_(data),
        end_(data + size) {}

protected:
    const uint8_t* start() const { return start_; };
    const uint8_t* end() const { return end_; };

    void mark() { mark_ = p_; }

    template <typename... Args>
    void error(const char* msg, Args... args)
    {
        Checker::error(mark_ - start_, msg, args...);
    }

    template <typename... Args>
    void warning(const char* msg, Args... args)
    {
        Checker::warning(mark_ - start_, msg, args...);
    }

    template <typename... Args>
    void error(const uint8_t *p, const char* msg, Args... args)
    {
        Checker::error(p - start_, msg, args...);
    }

    template <typename... Args>
    void warning(const uint8_t *p, const char* msg, Args... args)
    {
        Checker::warning(p - start_, msg, args...);
    }

    template <typename... Args>
    void fatal(const uint8_t *p, const char* msg, Args... args)
    {
        Checker::fatal(p - start_, msg, args...);
        // throw ValueException(msg, args...);        // TODO
    }

    void checkTruncated()
    {
        if(p_ >= end_) fatal(end_, "File truncated");
    }

    uint32_t readVarint32()
    {
        checkTruncated();
        const uint8_t* p = p_;
        try
        {
            return clarisma::safeReadVarint32(p_, end_);
        }
        catch(std::runtime_error& e)
        {
            fatal(p, e.what());
            return 0;
        }
    }

    uint64_t readVarint64()
    {
        checkTruncated();
        const uint8_t* p = p_;
        try
        {
            return clarisma::safeReadVarint64(p_, end_);
        }
        catch(std::runtime_error& e)
        {
            fatal(p, e.what());
            return 0;
        }
    }

    const ShortVarString* readString()
    {
        mark();
        const ShortVarString* str = reinterpret_cast<const ShortVarString*>(p_);
        uint32_t length = readVarint32();
        if(length >= 1 << 14)
        {
            error("Excessive string length (%d)", length);
            return nullptr;
        }
        p_ += length;
        checkTruncated();
        return str;
    }

    bool checkRange(const char* type, uint32_t code, size_t maxPlusOne)
    {
        if(code >= maxPlusOne)
        {
            error("%s #%d exceeds maximum (%zu)", type, code,
                static_cast<int64_t>(maxPlusOne) - 1);
            return false;
        }
        return true;
    }

    const uint8_t* p_;
    const uint8_t* mark_;
    const uint8_t* start_;
    const uint8_t* end_;
};

} // namespace clarisma
