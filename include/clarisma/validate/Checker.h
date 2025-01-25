// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <string>
#include <clarisma/text/Format.h>


namespace clarisma {

class Checker
{
public:
    class Error
    {
    public:
      // TODO: "ERROR" is defined as a macro (breaks Updater.cpp)
        enum class Severity { INFO, WARNING, NONFATAL_ERROR, FATAL };

        Error(uint64_t location, Severity severity, const std::string& message) :
            locationAndSeverity_(location | (static_cast<uint64_t>(severity) << 62)),
            message_(message)
        {
        }

        uint64_t location() const  { return locationAndSeverity_ & 0x3fff'ffff'ffff'ffffULL; }
        Severity severity() const { return static_cast<Severity>(locationAndSeverity_ >> 62); }
        const std::string& message() const  { return message_; }

    private:
        uint64_t locationAndSeverity_;
        std::string message_;
    };

    const std::vector<Error>& errors() const  { return errors_; }

    template <typename... Args>
    void error(uint64_t location, Error::Severity severity,
        const char* msg, Args... args)
    {
        errors_.emplace_back(location, severity, Format::format(msg, args...));
    }

    template <typename... Args>
    void error(uint64_t location, const char* msg, Args... args)
    {
        error(location, Error::Severity::NONFATAL_ERROR, msg, args...);
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