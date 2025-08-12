// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdint>
#include <memory>

#include "clarisma/util/enum.h"
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
// Prevent Windows headers from clobbering min/max
#define NOMINMAX
#endif
#include <windows.h>
#endif 

namespace clarisma {

class FileHandle
{
public:
#ifdef _WIN32
    using Native = HANDLE;
    static constexpr Native INVALID = INVALID_HANDLE_VALUE;
#else
    using Native = int;
    static constexpr Native INVALID = -1;
#endif

    enum class OpenMode
    {
        READ = 1,                   // Access
        WRITE = 2,
        CREATE = 4,                 // Creation
        REPLACE_EXISTING = 8,
        TEMPORARY = 16,             // Special
        DELETE_ON_CLOSE = 32
    };

    Native native() const noexcept { return handle_; }

    bool tryOpen(const char* fileName, OpenMode mode);

    [[nodiscard]] bool tryGetSize(uint64_t& size) const noexcept;
    uint64_t getSize() const;
    [[nodiscard]] bool trySetSize(uint64_t newSize) noexcept;
    void setSize(uint64_t newSize);
    void expand(uint64_t newSize);
    void truncate(uint64_t newSize);
    uint64_t allocatedSize() const;

    [[nodiscard]] bool trySeek(uint64_t posAbsolute) noexcept;
    void seek(uint64_t posAbsolute);

    [[nodiscard]] bool tryRead(void* buf, size_t length, size_t& bytesRead) noexcept;
    void read(void* buf, size_t length, size_t& bytesRead);
    [[nodiscard]] bool tryReadAll(void* buf, size_t length) noexcept;
    void readAll(void* buf, size_t length);
    [[nodiscard]] bool tryReadAt(uint64_t ofs, void* buf, size_t length, size_t& bytesRead) const noexcept;
    void readAt(uint64_t ofs, void* buf, size_t length, size_t& bytesRead) const;
    [[nodiscard]] bool tryReadAllAt(uint64_t ofs, void* buf, size_t length) const noexcept;
    void readAllAt(uint64_t ofs, void* buf, size_t length) const;

    /// @brief Reads the given number of bytes at `ofs`. If all bytes
    /// were read successfully, returns a typed pointer to a heap-
    /// allocated buffer, otherwise nullptr.
    ///
    template<typename T>
    [[nodiscard]] std::unique_ptr<T[]> tryReadAllAt(uint64_t ofs, size_t length);

    /// @brief Reads the given number of bytes at `ofs`. If all bytes
    /// were read successfully, returns a typed pointer to a heap-
    /// allocated buffer, otherwise throws IOException.
    ///
    template<typename T>
    [[nodiscard]] std::unique_ptr<T[]> readAllAt(uint64_t ofs, size_t length);

    /// @brief Attempts to write `length` bytes from the given buffer.
    ///
    [[nodiscard]] bool tryWrite(const void* buf, size_t length, size_t& bytesWritten) noexcept;
    void write(const void* buf, size_t length, size_t& bytesWritten);
    [[nodiscard]] bool tryWriteAll(const void* buf, size_t length) noexcept;
    void writeAll(const void* buf, size_t length);
    [[nodiscard]] bool tryWriteAt(uint64_t ofs, const void* buf, size_t length, size_t& bytesWritten) noexcept;
    void writeAt(uint64_t ofs, const void* buf, size_t length, size_t& bytesWritten);
    [[nodiscard]] bool tryWriteAllAt(uint64_t ofs, const void* buf, size_t length) noexcept;
    void writeAllAt(uint64_t ofs, const void* buf, size_t length);

    template <typename C>
    [[nodiscard]] bool tryWriteAll(const C& c) noexcept
    {
        return tryWriteAll(c.data(), c.size() * sizeof(C::value_type));
    }

    template <typename C>
    void writeAll(const C& c)
    {
        return writeAll(c.data(), c.size() * sizeof(C::value_type));
    }

    template <typename C>
    [[nodiscard]] bool tryWriteAllAt(uint64_t ofs, const C& c) noexcept
    {
        return tryWriteAllAt(ofs, c.data(), c.size() * sizeof(C::value_type));
    }

    template <typename C>
    void writeAllAt(uint64_t ofs, const C& c)
    {
        writeAllAt(ofs, c.data(), c.size() * sizeof(C::value_type));
    }

    [[nodiscard]] bool trySyncData() noexcept;
    void syncData();
    [[nodiscard]] bool trySync() noexcept;
    void sync();

protected:
    Native handle_ = INVALID;
};

} // namespace clarisma

CLARISMA_ENUM_FLAGS(FileHandle::OpenMode)

#if defined(_WIN32)
#include "detail/FileHandle_win.inl"
#else
#include "detail/FileHandle_posix.inl"
#endif