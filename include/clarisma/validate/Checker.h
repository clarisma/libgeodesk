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

class Checker
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

private:
    std::vector<Error> errors_;
};

} // namespace clarisma