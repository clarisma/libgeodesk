// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "FileHandle.h"
#include <filesystem>
#include "clarisma/alloc/Block.h"
// #include <clarisma/alloc/Block.h>

namespace clarisma {

class File : public FileHandle
{
public:
    File() = default;
    explicit File(Native handle) : FileHandle(handle) {};
    File(const File& other) = delete;
    File(File&& other) noexcept
    {
        handle_ = other.handle_;
        other.handle_ = INVALID;
    }

    ~File() 
    {
        close();
    }

    File& operator=(const File& other) = delete;
    File& operator=(File&& other) noexcept
    {
        if (this != &other)
        {
            handle_ = other.handle_;
            other.handle_ = INVALID;
        }
        return *this;
    }

    std::string fileName() const;

    // TODO: deprecated
    static std::string path(FileHandle h) { return h.fileName(); }

    ByteBlock readBlock(size_t length);

    static bool exists(const char* fileName);
    static void remove(const char* fileName);
    static void rename(const char* from, const char* to);

    /*
    static std::string path(FileHandle handle);
    std::string path() const { return path(fileHandle_); }
    */

    static ByteBlock readAll(const char* filename);
    static ByteBlock readAll(const std::filesystem::path& path)
    {
        std::string pathStr = path.string();
        return readAll(pathStr.c_str());
    }
    static std::string readString(const char* filename);
    static std::string readString(const std::filesystem::path& path)
    {
        std::string pathStr = path.string();
        return readString(pathStr.c_str());
    }
    static void writeAll(const char* filename, const void* data, size_t size);
    static void writeAll(const std::filesystem::path& path, const void* data, size_t size)
    {
        std::string pathStr = path.string();
        writeAll(pathStr.c_str(), data, size);
    }
    template<typename T>
    static void writeAll(const char* filename, T span)
    {
        writeAll(filename, span.data(), span.size());
    }
    template<typename T>
    static void writeAll(const std::filesystem::path& path, T span)
    {
        std::string pathStr = path.string();
        writeAll(pathStr.c_str(), span);
    }

    /*
    bool tryLock(uint64_t ofs, uint64_t length, bool shared = false);
    bool tryLockShared(uint64_t ofs, uint64_t length)
    {
        return tryLock(ofs, length, true);
    }
    bool tryLockExclusive(uint64_t ofs, uint64_t length)
    {
        return tryLock(ofs, length, false);
    }
    bool tryUnlock(uint64_t ofs, uint64_t length);
    */
};


} // namespace clarisma
