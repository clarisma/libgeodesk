// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <clarisma/io/FileError.h>
#include <clarisma/util/enum.h>
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

using std::byte;

namespace clarisma {

/// @brief Lightweight wrapper for a native file handle on Windows,
/// Linux and macOS.
///
/// - try... methods will return true/false to indicate success or
///   failure (Caller can then retrieve the last error). All others
///   will throw IOException upon failure.
/// - ...read/write...At methods are positional; on Windows, these
///   methods are always synchronous (FileHandle::open does not
///   support FILE_FLAG_OVERLAPPED)
/// - ...readAll/writeAll... methods ensure all requested bytes are
///   read/written unless EOF or an error occurs; regular read and
///   write methods may read or write fewer bytes than requested,
///   if no error occurred or EOF is reached
///
/// TODO: Support
/// - FILE_FLAG_NO_BUFFERING
/// - FILE_FLAG_RANDOM_ACCESS
///
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

    FileHandle() = default;
    FileHandle(Native native) : handle_(native) {}

    /// @brief Copyable: copies the same native handle (no ownership).
    FileHandle(const FileHandle&) = default;

    /// @brief Move-construct; steals the native handle and invalidates @p other.
    FileHandle(FileHandle&& other) noexcept :
        handle_(other.handle_)
    {
        other.handle_ = INVALID;
    }

    /// @brief Copy-assign: aliases the same native handle (no ownership).
    FileHandle& operator=(const FileHandle&) = default;

    /// @brief Move-assign; steals the native handle and invalidates @p other.
    FileHandle& operator=(FileHandle&& other) noexcept
    {
        if (this != &other)
        {
            handle_ = other.handle_;
            other.handle_ = INVALID;
        }
        return *this;
    }

    // Keep the values of READ,WRITE and CREATE stable for
    // use in other classes
    // Bits 0-5 are used for value mapping in open(), keep values in sync

    enum class OpenMode
    {
        /// @brief Open file for reading
        READ = 1 << 0,                   // Access

        /// @brief Open file for writing
        WRITE = 1 << 1,

        /// @brief Create file if it does not exist
        CREATE = 1 << 2,                 // Creation

        /// @brief File must be new; used
        /// with CREATE, fails if file exists
        NEW = 1 << 3,

        /// @brief Truncate any existing file
        /// (typically used with CREATE, opposite of NEW)
        TRUNCATE = 1 << 4,

        /// @brief The file's lifetime (hint for caching;
        /// only used by Windows, ignored by others)
        TEMPORARY = 1 << 5,

        /// @brief File is deleted once its last open
        /// handle is closed
        DELETE_ON_CLOSE = 1 << 6,

        /// Create as sparse file. Only used by Windows
        /// (Linux/MacOS ignores this flag since files
        /// are sparse by default)
        SPARSE = 1 << 7

        // TODO: If adding more flags, modify Store::OpenMode !!!
    };

    FileHandle handle() const noexcept { return handle_; }
    Native native() const noexcept { return handle_; }

    static FileError error();
    static std::string errorMessage();
    static std::string errorMessage(const char* fileName);

    bool tryOpen(const char* fileName, OpenMode mode) noexcept;
    void open(const char* fileName, OpenMode mode);
    void open(const std::filesystem::path& path, OpenMode mode)
    {
        std::string pathStr = path.string();
        open(pathStr.c_str(), mode);
    }

    bool tryClose() noexcept;
    void close();
    bool isOpen() const noexcept { return handle_ != INVALID; };

    [[nodiscard]] bool tryGetSize(uint64_t& size) const noexcept;
    [[nodiscard]] uint64_t getSize() const;
    [[nodiscard]] uint64_t size() const { return getSize(); }
    [[nodiscard]] bool trySetSize(uint64_t newSize) noexcept;
    void setSize(uint64_t newSize);
    void expand(uint64_t newSize);
    void truncate(uint64_t newSize);
    [[nodiscard]] uint64_t allocatedSize() const;

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

    /// @brief Reads the given number of bytes from current offset.
    /// If all bytes were read successfully, returns a typed
    /// pointer to a heap-allocated buffer, otherwise throws IOException.
    ///
    template<typename T>
    [[nodiscard]] std::unique_ptr<T[]> readAll(size_t length)
    {
        static_assert(std::is_trivially_copyable_v<T>,
        "T must be trivially copyable for binary I/O.");
        assert(length % sizeof(T) == 0);
        const size_t count = length / sizeof(T);
        auto buf = std::make_unique<T[]>(count);
        readAll(static_cast<void*>(buf.get()), length);
        return buf;
    }

    /// @brief Reads the given number of bytes at `ofs`. If all bytes
    /// were read successfully, returns a typed pointer to a heap-
    /// allocated buffer, otherwise throws IOException.
    ///
    template<typename T>
    [[nodiscard]] std::unique_ptr<T[]> readAllAt(uint64_t ofs, size_t length)
    {
        static_assert(std::is_trivially_copyable_v<T>,
        "T must be trivially copyable for binary I/O.");
        assert(length % sizeof(T) == 0);
        const size_t count = length / sizeof(T);
        auto buf = std::make_unique<T[]>(count);
        readAllAt(ofs, static_cast<void*>(buf.get()), length);
        return buf;
    }

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
        return writeAll(c.data(), c.size() * sizeof(typename C::value_type));
    }

    template <typename C>
    [[nodiscard]] bool tryWriteAllAt(uint64_t ofs, const C& c) noexcept
    {
        return tryWriteAllAt(ofs, c.data(), c.size() * sizeof(typename C::value_type));
    }

    template <typename C>
    void writeAllAt(uint64_t ofs, const C& c)
    {
        writeAllAt(ofs, c.data(), c.size() * sizeof(typename C::value_type));
    }

    // TODO: Allow these forms for now for backward compatibility
    size_t write(const void* buf, size_t length)
    {
        size_t written;
        write(buf, length, written);
        return written;
    }

    size_t read(void* buf, size_t length)
    {
        size_t bytesRead;
        read(buf, length, bytesRead);
        return bytesRead;
    }

    size_t read(uint64_t ofs, void* buf, size_t length)
    {
        size_t bytesRead;
        readAt(ofs, buf, length, bytesRead);
        return bytesRead;
    }

    template <typename C>
    void write(const C& c)
    {
        return writeAll(c);
    }

    void force() { sync(); }

    // ===== end compatibility =====

    [[nodiscard]] bool trySyncData() noexcept;
    void syncData();
    [[nodiscard]] bool trySync() noexcept;
    void sync();

    std::string fileName() const;

    void makeSparse();
    void allocate(uint64_t ofs, size_t length);
    void deallocate(uint64_t ofs, size_t length);
    void zeroFill(uint64_t ofs, size_t length);

    /// @brief Attempts to lock the specified region without
    /// blocking.
    ///
    /// @param ofs      start of the region
    /// @param length   length of the region
    /// @param shared   false for exclusive (write), true for shared (read)
    ///
    bool tryLock(uint64_t ofs, uint64_t length, bool shared = false);
    bool tryLockShared(uint64_t ofs, uint64_t length)
    {
        return tryLock(ofs, length, true);
    }
    bool tryLockExclusive(uint64_t ofs, uint64_t length)
    {
        return tryLock(ofs, length, false);
    }

    /// @brief Unlocks the given region.
    ///
    /// On Windows, trying to unlock a region that hasn't been
    /// locked results in ERROR_NOT_LOCKED.
    ///
    bool tryUnlock(uint64_t ofs, uint64_t length);

    byte* map(uint64_t offset, uint64_t length, bool writable = false);
    static void unmap(void* address, uint64_t length);

protected:
    Native handle_ = INVALID;
};

CLARISMA_ENUM_FLAGS(FileHandle::OpenMode)

} // namespace clarisma

#if defined(_WIN32)
#include "detail/FileHandle_win.inl"
#else
#include "detail/FileHandle_posix.inl"
#endif