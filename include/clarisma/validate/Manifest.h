// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <clarisma/text/Format.h>
#include <clarisma/util/ShortVarString.h>
#include <clarisma/util/varint_safe.h>


namespace clarisma {

class Manifest 
{
public:
    class Error
    {
    public:
        enum class Severity { INFO, WARNING, ERROR, FATAL };

        Error(uint64_t location, Severity severity, const std::string& message) :
            locationAndSeverity_(location | (static_cast<uint64_t>(severity) << 62)),
            message_(message)
        {
        }

    private:
        uint64_t locationAndSeverity_;
        std::string message_;
    };

    const std::vector<Error>& errors() const  { return errors_; };

    template <typename... Args>
    void error(uint64_t location, Error::Severity severity,
        const char* msg, Args... args);

    template <typename... Args>
    void error(uint64_t location, const char* msg, Args... args)
    {
        error(location, Error::Severity::ERROR, msg, args...);
    }

    template <typename... Args>
    void warning(uint64_t location, const char* msg, Args... args)
    {
        error(location, Error::Severity::WARNING, msg, args...);
    }

    template <typename... Args>
    void fatal(uint64_t location, const char* msg, Args... args)
    {
        error(location, Error::Severity::FATAL, msg, args...);
    }

    template<typename T>
    class Reader
    {
    public:
        Reader(T& manifest) :
            p_(manifest.start_),
            mark_(manifest.start_),
            manifest_(manifest) {}

    protected:
        void mark() { mark_ = p_; }

        template <typename... Args>
        void error(const char* msg, Args... args)
        {
            manifest_.error(mark_ - manifest_.start_, msg, args...);
        }

        template <typename... Args>
        void warning(const char* msg, Args... args)
        {
            manifest_.warning(mark_ - manifest_.start_, msg, args...);
        }

        template <typename... Args>
        void error(const uint8_t *p, const char* msg, Args... args)
        {
            manifest_.error(p - manifest_.start_, msg, args...);
        }

        template <typename... Args>
        void warning(const uint8_t *p, const char* msg, Args... args)
        {
            manifest_.warning(p - manifest_.start_, msg, args...);
        }

        template <typename... Args>
        void fatal(const uint8_t *p, const char* msg, Args... args)
        {
            manifest_.fatal(p - manifest_.start_, msg, args...);
            throw ValueException(msg, args...);
        }

        void checkTruncated()
        {
            if(p_ >= manifest_.end_) fatal(manifest_.end_, "File truncated");
        }

        uint32_t readVarint32()
        {
            checkTruncated();
            const uint8_t* p = p_;
            try
            {
                return clarisma::safeReadVarint32(p_, manifest_.end_);
            }
            catch(std::runtime_error& e)
            {
                fatal(p, e.what());
                return 0;
            }
        }

        uint32_t readVarint64()
        {
            checkTruncated();
            const uint8_t* p = p_;
            try
            {
                return clarisma::safeReadVarint64(p_, manifest_.end_);
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
            }
            p_ += length;
            checkTruncated();
            return str;
        }

        bool checkRange(const char* type, uint32_t code, size_t maxPlusOne)
        {
            if(code >= maxPlusOne)
            {
                error("%s #%d exceeds maximum (%ll)", type, code,
                    static_cast<int64_t>(maxPlusOne) - 1);
                return false;
            }
            return true;
        }

        const uint8_t* p_;
        const uint8_t* mark_;
        T& manifest_;
    };

    Manifest(const uint8_t* start, size_t size) :
        start_(start),
        end_(start + size) {}

    const uint8_t* start() const { return start_; };
    const uint8_t* end() const { return end_; };

private:
    const uint8_t* start_;
    const uint8_t* end_;
    std::vector<Error> errors_;
};

} // namespace clarisma