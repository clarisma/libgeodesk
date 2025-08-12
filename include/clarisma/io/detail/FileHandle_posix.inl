// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

// POSIX inline implementations for clarisma::FileHandle

#include <cstddef>
#include <cerrno>
#include <unistd.h>       // read, write, pread, pwrite, ftruncate, fsync
#include <sys/stat.h>     // fstat
#include <sys/types.h>    // off_t
#include <clarisma/io/IOException.h>

static_assert(sizeof(off_t) >= 8, "off_t must be 64-bit");
static_assert(sizeof(ssize_t) >= 8, "ssize_t must be 64-bit");

namespace clarisma
{

inline bool FileHandle::tryOpen(const char* fileName, OpenMode mode)
{
    static DWORD ACCESS_MODES[] =
    {
        O_RDONLY,         // none (default to READ)
        O_RDONLY,         // READ
        O_WRONLY,         // WRITE
        O_RDWR            // READ + WRITE
    };

    // Access mode: use bits 0 & 1 of OpenMode
    int flags = ACCESS_MODES[static_cast<int>(mode) & 3];

    static DWORD CREATE_MODES[] =
    {
        0,                   // none (default: open if exists)
        O_CREAT,             // CREATE
        O_TRUNC,             // REPLACE_EXISTING (implies CREATE)
        O_CREAT | O_TRUNC,   // CREATE + REPLACE_EXISTING
    };

    // Create disposition: use bits 4 & 5 of OpenMode
    flags |= CREATE_MODES[(static_cast<int>(mode) >> 2) & 3];

    // Ignore TEMPORARY flag (Windows only)
    // TODO: Decide what to do with DELETE_ON_CLOSE

    // Sparse files are inherently supported on most UNIX filesystems.
    // You don't need to specify a special flag, just don't write to all parts of the file.

    handle_ = ::open(fileName, flags, 0666);
    return handle_ != INVALID;
}

/// @brief Get logical file size.
/// @return true on success, false on error (no throw)
inline bool FileHandle::tryGetSize(uint64_t& size) const noexcept
{
    struct stat st;
    int res = ::fstat(handle_, &st);
    size = res == 0 ? static_cast<uint64_t>(st.st_size) : 0;
    return res == 0;
}

/// @brief Get logical file size or throw on error.
inline uint64_t FileHandle::getSize() const
{
    uint64_t s;
    if (!tryGetSize(s))
    {
        IOException::checkAndThrow();
    }
    return s;
}

/// @brief Set logical file size (does not change pointer).
inline bool FileHandle::trySetSize(uint64_t newSize) noexcept
{
    // Assume 64-bit off_t
    return ::ftruncate(handle_, static_cast<off_t>(newSize)) == 0;
}

/// @brief Set logical file size or throw on error.
inline void FileHandle::setSize(uint64_t newSize)
{
    if (!trySetSize(newSize))
    {
        IOException::checkAndThrow();
    }
}

/// @brief Grow file to at least newSize.
inline void FileHandle::expand(uint64_t newSize)
{
    if (newSize > getSize())
    {
        setSize(newSize);
    }
}

/// @brief Truncate file to newSize.
inline void FileHandle::truncate(uint64_t newSize)
{
    setSize(newSize);
}

/// @brief Get allocated (on-disk) size. Throws on error.
inline uint64_t FileHandle::allocatedSize() const
{
    struct stat st;
    if (::fstat(handle_, &st) != 0)
    {
        IOException::checkAndThrow();
    }
    // POSIX st_blocks is in 512-byte units.
    return static_cast<uint64_t>(st.st_blocks) * 512ull;
}

/// @brief Seek to absolute position using file pointer.
inline bool FileHandle::trySeek(uint64_t posAbsolute) noexcept
{
    off_t rc = ::lseek(handle_, static_cast<off_t>(posAbsolute), SEEK_SET);
    return rc != static_cast<off_t>(-1);
}

/// @brief Seek or throw on error.
inline void FileHandle::seek(uint64_t posAbsolute)
{
    if (!trySeek(posAbsolute))
    {
        IOException::checkAndThrow();
    }
}

/// @brief One-shot read at current pointer (no loop).
inline bool FileHandle::tryRead(
    void* buf, size_t length, size_t& bytesRead) noexcept
{
    ssize_t n = ::read(handle_, buf, length);
    bytesRead = n < 0 ? 0 : static_cast<size_t>(n);
    return n >= 0;
}

/// @brief Read once; throws on OS error. May return short at EOF.
inline void FileHandle::read(
    void* buf, size_t length, size_t& bytesRead)
{
    if (!tryRead(buf, length, bytesRead))
    {
        IOException::checkAndThrow();
    }
}

/// @brief Loop until length or EOF; no throw on clean EOF.
inline bool FileHandle::tryReadAll(void* buf, size_t length) noexcept
{
    std::byte* p = static_cast<std::byte*>(buf);
    size_t total = 0;

    while (total < length)
    {
        ssize_t n = ::read(handle_, p + total, length - total);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0)
        {
            return false; // EOF before full read
        }
        total += static_cast<size_t>(n);
    }
    return true;
}

/// @brief Loop until length; throw on error or EOF before length.
inline void FileHandle::readAll(void* buf, size_t length)
{
    if (!tryReadAll(buf, length))
    {
        IOException::checkAndThrow();
    }
}

/// @brief One-shot positional read.
inline bool FileHandle::tryReadAt(
    uint64_t ofs, void* buf, size_t length, size_t& bytesRead) const noexcept
{
    ssize_t n = ::pread(
        handle_, buf, length, static_cast<off_t>(ofs));
    bytesRead = n < 0 ? 0 : static_cast<size_t>(n);
    return n >= 0;
}

/// @brief Positional read once; throw on OS error.
inline void FileHandle::readAt(
    uint64_t ofs, void* buf, size_t length, size_t& bytesRead) const
{
    if (!tryReadAt(ofs, buf, length, bytesRead))
    {
        IOException::checkAndThrow();
    }
}

/// @brief Loop positional until length or EOF; no throw on EOF.
inline bool FileHandle::tryReadAllAt(
    uint64_t ofs, void* buf, size_t length) const noexcept
{
    std::byte* p = static_cast<std::byte*>(buf);
    size_t total = 0;

    while (total < length)
    {
        ssize_t n = ::pread(
            handle_, p + total, length - total,
            static_cast<off_t>(ofs + total));
        if (n < 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0)
        {
            return false; // EOF before full read
        }
        total += static_cast<size_t>(n);
    }
    return true;
}

/// @brief Loop positional until length; throw on error/EOF before length.
inline void FileHandle::readAllAt(
    uint64_t ofs, void* buf, size_t length) const
{
    if (!tryReadAllAt(ofs, buf, length))
    {
        IOException::checkAndThrow();
    }
}

/// @brief One-shot write at current pointer.
inline bool FileHandle::tryWrite(
    const void* buf, size_t length, size_t& bytesWritten) noexcept
{
    ssize_t n = ::write(handle_, buf, length);
    bytesWritten = n < 0 ? 0 : static_cast<size_t>(n);
    return n >= 0;
}

/// @brief One-shot write; throw on OS error.
inline void FileHandle::write(
    const void* buf, size_t length, size_t& bytesWritten)
{
    if (!tryWrite(buf, length, bytesWritten))
    {
        IOException::checkAndThrow();
    }
}

/// @brief Loop until all bytes written; no throw on error (false).
inline bool FileHandle::tryWriteAll(const void* buf, size_t length) noexcept
{
    const std::byte* p = static_cast<const std::byte*>(buf);
    size_t total = 0;

    while (total < length)
    {
        ssize_t n = ::write(handle_, p + total, length - total);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0 && (length - total) != 0)
        {
            return false; // no progress
        }
        total += static_cast<size_t>(n);
    }
    return true;
}

/// @brief Loop until all bytes written; throw on any failure.
inline void FileHandle::writeAll(const void* buf, size_t length)
{
    if (!tryWriteAll(buf, length))
    {
        IOException::checkAndThrow();
    }
}

/// @brief One-shot positional write.
inline bool FileHandle::tryWriteAt(
    uint64_t ofs, const void* buf, size_t length, size_t& bytesWritten) noexcept
{
    ssize_t n = ::pwrite(
        handle_, buf, length, static_cast<off_t>(ofs));
    bytesWritten = n < 0 ? 0 : static_cast<size_t>(n);
    return n >= 0;
}

/// @brief Positional write once; throw on OS error.
inline void FileHandle::writeAt(
    uint64_t ofs, const void* buf, size_t length, size_t& bytesWritten)
{
    if (!tryWriteAt(ofs, buf, length, bytesWritten))
    {
        IOException::checkAndThrow();
    }
}

/// @brief Loop positional until all bytes written; no throw on error (false).
inline bool FileHandle::tryWriteAllAt(
    uint64_t ofs, const void* buf, size_t length) noexcept
{
    const std::byte* p = static_cast<const std::byte*>(buf);
    size_t total = 0;

    while (total < length)
    {
        ssize_t n = ::pwrite(
            handle_, p + total, length - total,
            static_cast<off_t>(ofs + total));
        if (n < 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0 && (length - total) != 0)
        {
            return false; // no progress
        }
        total += static_cast<size_t>(n);
    }
    return true;
}

/// @brief Loop positional until all bytes; throw on any failure.
inline void FileHandle::writeAllAt(
    uint64_t ofs, const void* buf, size_t length)
{
    if (!tryWriteAllAt(ofs, buf, length))
    {
        IOException::checkAndThrow();
    }
}

/// @brief Flush file data only if available; else full metadata flush.
inline bool FileHandle::trySyncData() noexcept
{
#if defined(_POSIX_SYNCHRONIZED_IO) || defined(__APPLE__) || defined(__linux__)
    return ::fdatasync(handle_) == 0;
#else
    return ::fsync(handle_) == 0;
#endif
}

/// @brief Flush or throw on error.
inline void FileHandle::syncData()
{
    if (!trySyncData())
    {
        IOException::checkAndThrow();
    }
}

/// @brief Full metadata flush.
inline bool FileHandle::trySync() noexcept
{
    return ::fsync(handle_) == 0;
}

/// @brief Full metadata flush or throw.
inline void FileHandle::sync()
{
    if (!trySync())
    {
        IOException::checkAndThrow();
    }
}

} // namespace clarisma
