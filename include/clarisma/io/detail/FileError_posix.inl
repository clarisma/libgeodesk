// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

// POSIX inline implementations for clarisma::FileHandle

#pragma once
#include <cstdint>
#include <cerrno>

namespace clarisma
{

/// \brief POSIX file/IO error codes (superset).
/// \details Each enumerator equals a native errno (int). Distinct
///          Windows cases may share the same errno here.
enum class FileError : uint32_t
{
    OK                      = 0,

    // Not found variants map to the same errno
    NOT_FOUND               = ENOENT,
    PATH_NOT_FOUND          = ENOENT,

    // Already exists variants map to the same errno
    ALREADY_EXISTS          = EEXIST,
    FILE_EXISTS             = EEXIST,

    // Permissions / access
    PERMISSION_DENIED       = EACCES,   // EPERM also occurs; EACCES chosen
    READ_ONLY_FILESYSTEM    = EROFS,

    // Path / name / handle
    NAME_TOO_LONG           = ENAMETOOLONG,
    INVALID_NAME            = EINVAL,   // best available analogue
    INVALID_HANDLE          = EBADF,
    NOT_A_DIRECTORY         = ENOTDIR,
    IS_A_DIRECTORY          = EISDIR,

    // Busy / locking / sharing
    BUSY                    = EBUSY,
    LOCK_VIOLATION          = EACCES,   // or EAGAIN on some fcntl locks
    SHARING_VIOLATION       = EBUSY,    // closest analogue

    // Storage / I/O
    DISK_FULL               = ENOSPC,
    FILE_TOO_LARGE          = EFBIG,
    IO_ERROR                = EIO,
    IO_DEVICE_ERROR         = ENODEV,   // ENXIO also common; choose ENODEV

    // Cross-device / fs constraints
    CROSS_DEVICE_LINK       = EXDEV,

    // Timing / interruption
    TIMED_OUT               = ETIMEDOUT,
    INTERRUPTED             = EINTR,

    // Capability
    NOT_SUPPORTED           = ENOTSUP,  // EOPNOTSUPP often equal to ENOTSUP

    // Misc that often arise in FS ops
    DIRECTORY_NOT_EMPTY     = ENOTEMPTY
};

} // namespace clarisma