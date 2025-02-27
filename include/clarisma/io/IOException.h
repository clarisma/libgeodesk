// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <stdexcept>
#include <string>
#include <clarisma/text/Format.h>

namespace clarisma {

class IOException : public std::runtime_error 
{
public:
    explicit IOException(const char* message)
        : std::runtime_error(message) {}

    explicit IOException(const std::string& message)
        : std::runtime_error(message) {}

    template <typename... Args>
    explicit IOException(const char* message, Args... args)
        : std::runtime_error(Format::format(message, args...)) {}

    explicit IOException(int errorCode) :
        IOException(getMessage(errorCode)) {}

    explicit IOException(unsigned long errorCode) :
        IOException(getMessage(static_cast<int>(errorCode))) {}

    static std::string getMessage(int errorCode);

    /**
     * On Linux, this function must only be called if the caller
     * is certain that an error occurred (errno is set) -- typically,
     * because a system call returned -1.
     */
    static void checkAndThrow();
    static void alwaysThrow();
    // static void alwaysThrow(const char* msg);
};


class FileNotFoundException : public IOException
{
public:
    explicit FileNotFoundException(const char* filename)
        : IOException(std::string(filename) + ": File not found") {}
    explicit FileNotFoundException(std::string filename)
        : FileNotFoundException(filename.c_str()) {}
};



} // namespace clarisma
