// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

// TODO: outdated:
// Precondition for *At() methods:
// - Handle opened with FILE_FLAG_OVERLAPPED for true positional I/O.
//   Passing OVERLAPPED to a non-overlapped file handle fails with
//   ERROR_INVALID_PARAMETER.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <clarisma/io/IOException.h>

namespace clarisma {

inline void FileHandle::close()
{
    if (handle_ != INVALID)
    {
        CloseHandle(handle_);
        handle_ = INVALID;
    }
}

/// @brief Get logical file size.
/// @return true on success, false on error (no throw)
inline bool FileHandle::tryGetSize(uint64_t& size) const noexcept
{
    LARGE_INTEGER li;
    BOOL ok = GetFileSizeEx(handle_, &li);
    size = ok ? static_cast<uint64_t>(li.QuadPart) : 0;
    return ok;
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
    FILE_END_OF_FILE_INFO info
    {
        .EndOfFile = { .QuadPart = static_cast<LONGLONG>(newSize) }
    };
    return ::SetFileInformationByHandle(
        handle_, FileEndOfFileInfo, &info, sizeof(info)) != 0;
}

/// @brief Set logical file size or throw on error.
inline void FileHandle::setSize(uint64_t newSize)
{
    if (!trySetSize(newSize)) IOException::checkAndThrow();
}

/// @brief Grow file to at least newSize.
inline void FileHandle::expand(uint64_t newSize)
{
    if (newSize > getSize()) setSize(newSize);
}

/// @brief Truncate file to newSize (or no-op if already smaller).
inline void FileHandle::truncate(uint64_t newSize)
{
    setSize(newSize);
}

/// @brief Get allocated (on-disk) size. Throws on error.
inline uint64_t FileHandle::allocatedSize() const
{
    FILE_STANDARD_INFO info{};
    if (!::GetFileInformationByHandleEx(
            handle_,
            FileStandardInfo,
            &info,
            sizeof(info)))
    {
        IOException::checkAndThrow();
    }
    return static_cast<uint64_t>(info.AllocationSize.QuadPart);
}

/// @brief Seek to absolute position using file pointer.
inline bool FileHandle::trySeek(uint64_t posAbsolute) noexcept
{
    LARGE_INTEGER li{};
    li.QuadPart = static_cast<LONGLONG>(posAbsolute);
    return ::SetFilePointerEx(handle_, li, nullptr, FILE_BEGIN) != 0;
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
    DWORD got = 0;
    DWORD toRead = static_cast<DWORD>(
        (std::min)(length, static_cast<size_t>(MAXDWORD)));
    BOOL ok = ::ReadFile(handle_, buf, toRead, &got, nullptr);
    bytesRead = static_cast<size_t>(got);
    return ok;
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
        DWORD toRead = static_cast<DWORD>(
            (std::min)(length - total,
                static_cast<size_t>(MAXDWORD)));
        DWORD got = 0;

        if (!::ReadFile(handle_, p + total, toRead, &got, nullptr))
        {
            return false;
        }
        if (got == 0)
        {
            // EOF before full read
            return false;
        }
        total += static_cast<size_t>(got);
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

/// @brief One-shot positional read (OVERLAPPED).
inline bool FileHandle::tryReadAt(
    uint64_t ofs, void* buf, size_t length, size_t& bytesRead) const noexcept
{
    OVERLAPPED ovl
    {
        .Offset = static_cast<DWORD>(ofs & 0xFFFFFFFFull),
        .OffsetHigh = static_cast<DWORD>((ofs >> 32) & 0xFFFFFFFFull)
    };

    DWORD toRead = static_cast<DWORD>(
        (std::min)(length, static_cast<size_t>(MAXDWORD)));
    DWORD got = 0;

    BOOL ok = ::ReadFile(handle_, buf, toRead, &got, &ovl);
    /*
    if (!ok)
    {
        // TODO: Don't need this if file wasn't opened with
        //  FILE_FLAG_OVERLAPPED (which FileHandle::open does
        //  not support)

        DWORD err = ::GetLastError();
        if (err == ERROR_IO_PENDING)
        {
            // Complete the single request; not a retry.
            ok = ::GetOverlappedResult(handle_, &ovl, &got, TRUE);
        }
    }
    */
    bytesRead = static_cast<size_t>(got);
    return ok;
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
        OVERLAPPED ovl{};
        uint64_t cur = ofs + total;
        ovl.Offset = static_cast<DWORD>(cur & 0xFFFFFFFFull);
        ovl.OffsetHigh = static_cast<DWORD>((cur >> 32) & 0xFFFFFFFFull);

        DWORD toRead = static_cast<DWORD>(
            (std::min)(length - total,
                static_cast<size_t>(MAXDWORD)));
        DWORD got = 0;

        BOOL ok = ::ReadFile(handle_, p + total, toRead, &got, &ovl);
        if (!ok) return false;
        /*
        // TODO: Don't need this if file wasn't opened with
        //  FILE_FLAG_OVERLAPPED (which FileHandle::open does
        //  not support)
        if (!ok)
        {
            DWORD err = ::GetLastError();
            if (err == ERROR_IO_PENDING)
            {
                if (!::GetOverlappedResult(handle_, &ovl, &got, TRUE))
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        */

        if (got == 0)
        {
            return false; // EOF
        }
        total += static_cast<size_t>(got);
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
    DWORD toWrite = static_cast<DWORD>(
        (std::min)(length, static_cast<size_t>(MAXDWORD)));
    DWORD wrote = 0;

    BOOL ok = ::WriteFile(handle_, buf, toWrite, &wrote, nullptr);
    bytesWritten = static_cast<size_t>(wrote);
    return ok;
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

/// @brief Loop until all bytes written; no throw on partial? (false)
inline bool FileHandle::tryWriteAll(
    const void* buf, size_t length) noexcept
{
    const std::byte* p = static_cast<const std::byte*>(buf);
    size_t total = 0;

    while (total < length)
    {
        DWORD chunk = static_cast<DWORD>(
            (std::min)(length - total,
                static_cast<size_t>(MAXDWORD)));
        DWORD wrote = 0;

        if (!::WriteFile(handle_, p + total, chunk, &wrote, nullptr))
        {
            return false;
        }
        if (wrote == 0 && chunk != 0)
        {
            return false;
        }
        total += static_cast<size_t>(wrote);
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

/// @brief One-shot positional write (OVERLAPPED).
inline bool FileHandle::tryWriteAt(
    uint64_t ofs, const void* buf, size_t length, size_t& bytesWritten) noexcept
{
    OVERLAPPED ovl{};
    ovl.Offset = static_cast<DWORD>(ofs & 0xFFFFFFFFull);
    ovl.OffsetHigh = static_cast<DWORD>((ofs >> 32) & 0xFFFFFFFFull);

    DWORD chunk = static_cast<DWORD>(
        (std::min)(length, static_cast<size_t>(MAXDWORD)));
    DWORD wrote = 0;

    BOOL ok = ::WriteFile(handle_, buf, chunk, &wrote, &ovl);
    /*
    // TODO: Don't need this if file wasn't opened with
    //  FILE_FLAG_OVERLAPPED (which FileHandle::open does
    //  not support)

    if (!ok)
    {
        DWORD err = ::GetLastError();
        if (err == ERROR_IO_PENDING)
        {
            ok = GetOverlappedResult(handle_, &ovl, &wrote, TRUE);
        }
    }
    */
    bytesWritten = static_cast<size_t>(wrote);
    return ok;
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

/// @brief Loop positional until all bytes written; no throw on false.
inline bool FileHandle::tryWriteAllAt(
    uint64_t ofs, const void* buf, size_t length) noexcept
{
    const std::byte* p = static_cast<const std::byte*>(buf);
    size_t total = 0;

    while (total < length)
    {
        OVERLAPPED ovl{};
        uint64_t cur = ofs + total;
        ovl.Offset = static_cast<DWORD>(cur & 0xFFFFFFFFull);
        ovl.OffsetHigh = static_cast<DWORD>((cur >> 32) & 0xFFFFFFFFull);

        DWORD chunk = static_cast<DWORD>(
            (std::min)(length - total,
                static_cast<size_t>(MAXDWORD)));
        DWORD wrote = 0;

        BOOL ok = ::WriteFile(handle_, p + total, chunk, &wrote, &ovl);
        if (!ok) return false;
        /*
        // TODO: Don't need this if file wasn't opened with
        //  FILE_FLAG_OVERLAPPED (which FileHandle::open does
        //  not support)

        if (!ok)
        {
            DWORD err = ::GetLastError();
            if (err == ERROR_IO_PENDING)
            {
                DWORD xfer = 0;
                if (!::GetOverlappedResult(handle_, &ovl, &xfer, TRUE))
                {
                    return false;
                }
                wrote = xfer;
            }
            else
            {
                return false;
            }
        }
        */

        if (wrote == 0 && chunk != 0)
        {
            return false;
        }
        total += static_cast<size_t>(wrote);
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

/// @brief Flush file data/metadata; no data-only primitive on Win32.
inline bool FileHandle::trySyncData() noexcept
{
    return ::FlushFileBuffers(handle_) != 0;
}

/// @brief Flush or throw on error.
inline void FileHandle::syncData()
{
    if (!trySyncData())
    {
        IOException::checkAndThrow();
    }
}

/// @brief Same as syncData() on Windows.
inline bool FileHandle::trySync() noexcept
{
    return trySyncData();
}

/// @brief Same as syncData() on Windows.
inline void FileHandle::sync()
{
    syncData();
}

inline void FileHandle::deallocate(uint64_t ofs, size_t length)
{
    zeroFill(ofs, length);
}



inline std::string FileHandle::fileName() const
{
    TCHAR buf[MAX_PATH];
    DWORD res = GetFinalPathNameByHandle(handle_, buf, MAX_PATH, FILE_NAME_NORMALIZED);
    return std::string(res > 0 ? buf : "<invalid file>");
}

/*
inline void FileHandle::makeSparse()
{
    DWORD bytesReturned = 0;
    if(!DeviceIoControl(
        handle_,          // Handle to the file obtained with CreateFile
        FSCTL_SET_SPARSE,     // Control code for setting the file as sparse
        NULL,                 // No input buffer required
        0,                    // Input buffer size is zero since no input buffer
        NULL,                 // No output buffer required
        0,                    // Output buffer size is zero since no output buffer
        &bytesReturned,       // Bytes returned
        NULL))                // Not using overlapped I/O
    {
        IOException::checkAndThrow();
    }
}
*/

inline bool FileHandle::tryLock(uint64_t ofs, uint64_t length, bool shared)
{
    OVERLAPPED overlapped;
    overlapped.Offset = ofs & 0xFFFFFFFF;
    overlapped.OffsetHigh = ofs >> 32;
    DWORD lockFlags = shared ? LOCKFILE_FAIL_IMMEDIATELY : LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY;
    return LockFileEx(handle_, lockFlags, 0, length & 0xFFFFFFFF, length >> 32, &overlapped);
}

inline bool FileHandle::tryUnlock(uint64_t ofs, uint64_t length)
{
    OVERLAPPED overlapped;
    overlapped.Offset = ofs & 0xFFFFFFFF;
    overlapped.OffsetHigh = ofs >> 32;
    return UnlockFileEx(handle_, 0, length & 0xFFFFFFFF, length >> 32, &overlapped);
}

inline void* FileHandle::map(uint64_t offset, uint64_t length, bool writable)
{
    DWORD protect = writable ? PAGE_READWRITE : PAGE_READONLY;

    // We need to explicitly specify the maximum size to force Windows
    // to grow the file to that size in case we're mapping beyond the
    // current file size
    uint64_t maxSize = offset + length;
    HANDLE mappingHandle = CreateFileMappingA(handle_, NULL, protect,
        static_cast<DWORD>(maxSize >> 32), static_cast<DWORD>(maxSize), NULL);
    if (!mappingHandle)
    {
        IOException::checkAndThrow();
    }

    void* mappedAddress = MapViewOfFile(mappingHandle,
        writable ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ,
        (DWORD)((offset >> 32) & 0xFFFFFFFF),
        (DWORD)(offset & 0xFFFFFFFF), length);
    CloseHandle(mappingHandle);
    if (!mappedAddress)
    {
        IOException::checkAndThrow();
    }
    return mappedAddress;
}

inline void FileHandle::unmap(void* address, uint64_t /* length */)
{
    UnmapViewOfFile(address);
}

} // namespace clarisma
