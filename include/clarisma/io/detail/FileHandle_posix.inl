// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

// POSIX inline implementations for clarisma::FileHandle

#include <cstddef>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>       // read, write, pread, pwrite, ftruncate, fsync
#include <sys/mman.h>     // mmap
#include <sys/stat.h>     // fstat
#include <sys/types.h>    // off_t
#include <clarisma/io/IOException.h>

static_assert(sizeof(off_t) >= 8, "off_t must be 64-bit");
static_assert(sizeof(ssize_t) >= 8, "ssize_t must be 64-bit");

namespace clarisma
{

inline FileError FileHandle::error()
{
    return static_cast<FileError>(errno);
}

inline bool FileHandle::tryOpen(const char* fileName, OpenMode mode) noexcept
{
    static int ACCESS_MODES[] =
    {
        O_RDONLY,         // none (default to READ)
        O_RDONLY,         // READ
        O_WRONLY,         // WRITE
        O_RDWR            // READ + WRITE
    };

    // Access mode: use bits 0 & 1 of OpenMode
    int flags = ACCESS_MODES[static_cast<int>(mode) & 3];

    static int OPEN_MODES[] =
    {
        0,                  // none (default: open if exists)
        O_CREAT,            // CREATE
        O_CREAT | O_EXCL,   // NEW
        O_CREAT | O_EXCL,   // CREATE + NEW
        O_TRUNC,            // TRUNCATE (does not create)
        O_CREAT | O_TRUNC,  // CREATE + TRUNCATE
        O_CREAT | O_EXCL,   // NEW + TRUNCATE (nonsensical, fall back to non-replace create)
        O_CREAT | O_EXCL,   // CREATE + NEW + TRUNCATE (nonsensical, fall back to non-replace create)
    };

    // Create disposition: use bits 2, 3 & 4 of OpenMode
    flags |= OPEN_MODES[(static_cast<int>(mode) >> 2) & 7];

    // Ignore TEMPORARY flag (Windows only)

    // Sparse files are inherently supported on most UNIX filesystems.
    // You don't need to specify a special flag, just don't write to all parts of the file.

    handle_ = ::open(fileName, flags, 0666);

    if (has(mode, OpenMode::DELETE_ON_CLOSE))
    {
        if (handle_ != INVALID)
        {
            // Rare option, but no harm to conditionally call unlink,
            // compiler should omit it from inline if open() is called
            // with constexpr OpenMode and DELETE_ON_CLOSE is not set

            ::unlink(fileName);
            // DELETE_ON_CLOSE is on "best efforts" basis,
            // so we'll allow it to fail silently
            // TODO: review this
        }
    }
    return handle_ != INVALID;
}

inline bool FileHandle::tryClose() noexcept
{
    int res = ::close(handle_);
    handle_ = INVALID;
    return res == 0;
}

inline void FileHandle::close()
{
    if (!tryClose()) throw IOException();
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
    if (!tryGetSize(s)) throw IOException();
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
    if (!trySetSize(newSize)) throw IOException();
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
        throw IOException();
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
    if (!trySeek(posAbsolute)) throw IOException();
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
        throw IOException();
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
        throw IOException();
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
        throw IOException();
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
        throw IOException();
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
        throw IOException();
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
        throw IOException();
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
        throw IOException();
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
        throw IOException();
    }
}

/// @brief Flush file data only if available; else full metadata flush.
inline bool FileHandle::trySyncData() noexcept
{
#if defined(__APPLE__)
    // macOS: fdatasync() is not reliable/portable; use fsync().
    return ::fsync(handle_) == 0;
#elif defined(_POSIX_SYNCHRONIZED_IO) || defined(__linux__)
    return ::fdatasync(handle_) == 0;
#else
    return ::fsync(handle_) == 0;
#endif
}

/// @brief Flush or throw on error.
inline void FileHandle::syncData()
{
    if (!trySyncData()) throw IOException();
}

/// @brief Full metadata flush.
inline bool FileHandle::trySync() noexcept
{
    return ::fsync(handle_) == 0;
}

/// @brief Full metadata flush or throw.
inline void FileHandle::sync()
{
    if (!trySync()) throw IOException();
}

inline bool FileHandle::tryLock(uint64_t ofs, uint64_t length, bool shared)
{
    struct flock fl;
    fl.l_type = shared ? F_RDLCK : F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = ofs;
    fl.l_len = length;
    return fcntl(handle_, F_SETLK, &fl) >= 0;
}


inline bool FileHandle::tryUnlock(uint64_t ofs, uint64_t length)
{
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = ofs;
    fl.l_len = length;
    return fcntl(handle_, F_SETLK, &fl) >= 0;
}

inline byte* FileHandle::map(uint64_t offset, uint64_t length, bool writable)
{
    int prot = writable ? (PROT_READ | PROT_WRITE) : PROT_READ;
    void* mappedAddress = mmap(nullptr, length, prot, MAP_SHARED, handle_, offset);
    if (mappedAddress == MAP_FAILED)
    {
        throw IOException();
    }
    return static_cast<byte*>(mappedAddress);
}

inline void FileHandle::unmap(void* address, uint64_t length)
{
    munmap(address, length);
}


} // namespace clarisma
