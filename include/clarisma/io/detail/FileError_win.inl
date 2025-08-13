// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

// TODO: outdated:
// Precondition for *At() methods:
// - Handle opened with FILE_FLAG_OVERLAPPED for true positional I/O.
//   Passing OVERLAPPED to a non-overlapped file handle fails with
//   ERROR_INVALID_PARAMETER.

#pragma once
#include <cstdint>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace clarisma {

/// \brief Windows file/IO error codes (superset).
/// \details Each enumerator equals a native Win32 error code (DWORD).
enum class FileError : uint32_t
{
    OK                      = ERROR_SUCCESS,

    // Not found variants
    NOT_FOUND               = ERROR_FILE_NOT_FOUND,
    PATH_NOT_FOUND          = ERROR_PATH_NOT_FOUND,

    // Already exists variants
    ALREADY_EXISTS          = ERROR_ALREADY_EXISTS,
    FILE_EXISTS             = ERROR_FILE_EXISTS,

    // Permissions / access
    PERMISSION_DENIED       = ERROR_ACCESS_DENIED,
    READ_ONLY_FILESYSTEM    = ERROR_WRITE_PROTECT,

    // Path / name / handle
    NAME_TOO_LONG           = ERROR_FILENAME_EXCED_RANGE,
    INVALID_NAME            = ERROR_INVALID_NAME,
    INVALID_HANDLE          = ERROR_INVALID_HANDLE,
    NOT_A_DIRECTORY         = ERROR_DIRECTORY,         // used when dir expected
    IS_A_DIRECTORY          = ERROR_DIRECTORY,         // same code on Windows

    // Busy / locking / sharing
    BUSY                    = ERROR_BUSY,
    LOCK_VIOLATION          = ERROR_LOCK_VIOLATION,
    SHARING_VIOLATION       = ERROR_SHARING_VIOLATION,

    // Storage / I/O
    DISK_FULL               = ERROR_DISK_FULL,
    FILE_TOO_LARGE          = ERROR_DISK_FULL,         // closest Windows match
    IO_ERROR                = ERROR_GEN_FAILURE,
    IO_DEVICE_ERROR         = ERROR_IO_DEVICE,

    // Cross-device / fs constraints
    CROSS_DEVICE_LINK       = ERROR_NOT_SAME_DEVICE,

    // Timing / interruption
    TIMED_OUT               = WAIT_TIMEOUT,
    INTERRUPTED             = ERROR_OPERATION_ABORTED,

    // Capability
    NOT_SUPPORTED           = ERROR_NOT_SUPPORTED,

    // Misc that often arise in FS ops
    DIRECTORY_NOT_EMPTY     = ERROR_DIR_NOT_EMPTY,
};

} // namespace clarisma