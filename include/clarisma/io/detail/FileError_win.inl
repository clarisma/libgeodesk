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
#ifndef NOMINMAX
// Prevent Windows headers from clobbering min/max
#define NOMINMAX
#endif
#include <windows.h>

namespace clarisma {

/// \brief Windows file/IO error codes (superset).
/// \details Each enumerator equals a native Win32 error code (DWORD).
enum class FileError : uint32_t
{
    // Success / unknown
    OK                      = ERROR_SUCCESS,
    UNKNOWN                 = ERROR_GEN_FAILURE, // generic failure

    // --- Existence ----------------------------------------------------
    NOT_FOUND               = ERROR_FILE_NOT_FOUND,
    PATH_NOT_FOUND          = ERROR_PATH_NOT_FOUND,
    ALREADY_EXISTS          = ERROR_ALREADY_EXISTS,
    FILE_EXISTS             = ERROR_FILE_EXISTS,

    // --- Permissions / policy ----------------------------------------
    PERMISSION_DENIED       = ERROR_ACCESS_DENIED,
    READ_ONLY_FILESYSTEM    = ERROR_WRITE_PROTECT,
    NOT_SUPPORTED           = ERROR_NOT_SUPPORTED,

    // --- Path / name / handle ----------------------------------------
    NAME_TOO_LONG           = ERROR_FILENAME_EXCED_RANGE,
    INVALID_NAME            = ERROR_INVALID_NAME,
    INVALID_PATH            = ERROR_INVALID_NAME,     // closest analogue
    INVALID_HANDLE          = ERROR_INVALID_HANDLE,
    NOT_A_DIRECTORY         = ERROR_DIRECTORY,        // Win uses same code
    IS_A_DIRECTORY          = ERROR_DIRECTORY,

    // --- Busy / locking / sharing ------------------------------------
    BUSY                    = ERROR_BUSY,
    WOULD_BLOCK             = ERROR_BUSY,             // closest analogue
    LOCK_VIOLATION          = ERROR_LOCK_VIOLATION,
    SHARING_VIOLATION       = ERROR_SHARING_VIOLATION,
    NOT_LOCKED              = ERROR_NOT_LOCKED,       // Windows-specific

    // --- Storage / I/O -----------------------------------------------
    DISK_FULL               = ERROR_DISK_FULL,
    FILE_TOO_LARGE          = ERROR_DISK_FULL,        // no dedicated code
    IO_ERROR                = ERROR_GEN_FAILURE,
    IO_DEVICE_ERROR         = ERROR_IO_DEVICE,
    END_OF_FILE             = ERROR_HANDLE_EOF,

    // --- Cross-filesystem / special ----------------------------------
    CROSS_DEVICE_LINK       = ERROR_NOT_SAME_DEVICE,
    DIRECTORY_NOT_EMPTY     = ERROR_DIR_NOT_EMPTY,
    TEXT_FILE_BUSY          = ERROR_SUCCESS,          // no Win equivalent
    RESOURCE_LIMIT          = ERROR_TOO_MANY_OPEN_FILES, // rough analogue
    TIMED_OUT               = WAIT_TIMEOUT,
    INTERRUPTED             = ERROR_OPERATION_ABORTED
};

} // namespace clarisma