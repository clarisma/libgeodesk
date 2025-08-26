// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <stdexcept>
#include <string>
#include <clarisma/io/FileError.h>

namespace clarisma {

class IOException : public std::runtime_error 
{
public:
    IOException();

    explicit IOException(const char* message)
        : std::runtime_error(message) {}

    explicit IOException(const std::string& message)
        : std::runtime_error(message) {}

    // explicit IOException(FileError error);
#ifdef _WIN32
    static std::string getMessage(const char* moduleName, int errorCode);
#endif

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
