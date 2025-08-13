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
    // Success / unknown
    OK                      = 0,
    UNKNOWN                 = EIO,          // generic I/O failure

    // --- Existence ----------------------------------------------------
    NOT_FOUND               = ENOENT,
    PATH_NOT_FOUND          = ENOENT,       // same errno as NOT_FOUND
    ALREADY_EXISTS          = EEXIST,
    FILE_EXISTS             = EEXIST,

    // --- Permissions / policy ----------------------------------------
    PERMISSION_DENIED       = EACCES,       // EPERM also common
    READ_ONLY_FILESYSTEM    = EROFS,
    NOT_SUPPORTED           = ENOTSUP,      // EOPNOTSUPP often same

    // --- Path / name / handle ----------------------------------------
    NAME_TOO_LONG           = ENAMETOOLONG,
    INVALID_NAME            = EINVAL,       // closest analogue
    INVALID_PATH            = EINVAL,
    INVALID_HANDLE          = EBADF,
    NOT_A_DIRECTORY         = ENOTDIR,
    IS_A_DIRECTORY          = EISDIR,

    // --- Busy / locking / sharing ------------------------------------
    BUSY                    = EBUSY,
    WOULD_BLOCK             = EWOULDBLOCK,  // or EAGAIN
    LOCK_VIOLATION          = EACCES,       // some use EAGAIN; choose EACCES
    SHARING_VIOLATION       = EBUSY,        // closest analogue
    NOT_LOCKED              = 0,            // POSIX: unlocking unlocked succeeds

    // --- Storage / I/O -----------------------------------------------
    DISK_FULL               = ENOSPC,
    FILE_TOO_LARGE          = EFBIG,
    IO_ERROR                = EIO,
    IO_DEVICE_ERROR         = ENODEV,       // ENXIO also common
    END_OF_FILE             = 0,            // EOF is not an errno

    // --- Cross-filesystem / special ----------------------------------
    CROSS_DEVICE_LINK       = EXDEV,
    DIRECTORY_NOT_EMPTY     = ENOTEMPTY,
    TEXT_FILE_BUSY          = ETXTBSY,
    RESOURCE_LIMIT          = EMFILE,       // ENFILE also possible
    TIMED_OUT               = ETIMEDOUT,
    INTERRUPTED             = EINTR
};

} // namespace clarisma