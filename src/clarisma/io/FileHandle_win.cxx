// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/FileHandle.h>
#include <winioctl.h>   // For FSCTL_SET_SPARSE

namespace clarisma {

bool FileHandle::tryOpen(const char* fileName, OpenMode mode) noexcept
{
    static DWORD ACCESS_MODES[] =
    {
        GENERIC_READ,                   // none (default to READ)
        GENERIC_READ,                   // READ
        GENERIC_WRITE,                  // WRITE
        GENERIC_READ | GENERIC_WRITE    // READ + WRITE
    };

    // Access mode: use bits 0 & 1 of OpenMode
    DWORD access = ACCESS_MODES[static_cast<int>(mode) & 3];

    static DWORD CREATE_MODES[] =
    {
        OPEN_EXISTING,      // none (default: open if exists)
        OPEN_ALWAYS,        // CREATE
        TRUNCATE_EXISTING,  // REPLACE_EXISTING (does not create)
        CREATE_ALWAYS,      // CREATE + REPLACE_EXISTING
    };

    // Create disposition: use bits 2 & 3 of OpenMode
    DWORD creationDisposition = CREATE_MODES[
        (static_cast<int>(mode) >> 2) & 3];

    static DWORD ATTRIBUTE_FLAGS[] =
    {
        FILE_ATTRIBUTE_NORMAL,
        FILE_ATTRIBUTE_TEMPORARY,
        FILE_FLAG_DELETE_ON_CLOSE,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE
    };

    // Other attribute flags: use bits 4 & 5 of OpenMode
    DWORD attributes = ATTRIBUTE_FLAGS[
        (static_cast<int>(mode) >> 4) & 3];

    handle_ = CreateFileA(fileName, access,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, creationDisposition,
        attributes, NULL);

    // TODO: only do this if file did not exist
    if (has(mode, OpenMode::SPARSE))
    {
        if (handle_ != INVALID)
        {
            DWORD bytesReturned;
            /* return */ DeviceIoControl(handle_, FSCTL_SET_SPARSE,
                NULL, 0, NULL, 0, &bytesReturned, NULL);
            // TODO: Fix this, does not always succeed

        }
    }

    return handle_ != INVALID;
}


void FileHandle::open(const char* fileName, OpenMode mode)
{
    if (!tryOpen(fileName, mode))
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            throw FileNotFoundException(fileName);
        }
        IOException::checkAndThrow();
    }
}


void FileHandle::makeSparse()
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

void FileHandle::zeroFill(uint64_t ofs, size_t length)
{
    FILE_ZERO_DATA_INFORMATION zeroDataInfo;
    zeroDataInfo.FileOffset.QuadPart = ofs;
    zeroDataInfo.BeyondFinalZero.QuadPart = ofs + length;

    DWORD bytesReturned = 0;
    if (!DeviceIoControl(
        handle_,                 // handle to file
        FSCTL_SET_ZERO_DATA,         // dwIoControlCode
        &zeroDataInfo,               // input buffer
        sizeof(zeroDataInfo),        // size of input buffer
        NULL,                        // lpOutBuffer
        0,                           // nOutBufferSize
        &bytesReturned,              // lpBytesReturned
        NULL))                       // lpOverlapped
    {
        IOException::checkAndThrow();
    }
}



/*
void File::allocate(uint64_t ofs, size_t length)
{
    // TODO: does not really exist on Windows
}

void File::deallocate(uint64_t ofs, size_t length)
{
    zeroFill(ofs, length);
}



bool File::exists(const char* fileName)
{
    DWORD attributes = GetFileAttributesA(fileName);
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
        {
            // "true" errors
            IOException::checkAndThrow();
        }
        return false;
    }
    return true;
}

void File::remove(const char* fileName)
{
    if (!DeleteFile(fileName))
    {
        IOException::checkAndThrow();
    }
}

void File::rename(const char* from, const char* to)
{
    if (!MoveFileEx(from, to, MOVEFILE_REPLACE_EXISTING))
    {
        IOException::checkAndThrow();
    }
}

std::string File::path(FileHandle handle)
{
    // TODO: long-path support (res will be > MAX_PATH)
    char buf[MAX_PATH];
    DWORD res = GetFinalPathNameByHandleA(handle, buf, MAX_PATH, FILE_NAME_NORMALIZED);
    return std::string(buf, res);
}

*/


/*
bool File::tryLock(uint64_t ofs, uint64_t length, bool shared)
{
    OVERLAPPED overlapped;
    overlapped.Offset = ofs & 0xFFFFFFFF;
    overlapped.OffsetHigh = ofs >> 32;
    DWORD lockFlags = shared ? LOCKFILE_FAIL_IMMEDIATELY : LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY;
    return LockFileEx(fileHandle_, lockFlags, 0, length & 0xFFFFFFFF, length >> 32, &overlapped);
}

bool File::tryUnlock(uint64_t ofs, uint64_t length)
{
    OVERLAPPED overlapped;
    overlapped.Offset = ofs & 0xFFFFFFFF;
    overlapped.OffsetHigh = ofs >> 32;
    return UnlockFileEx(fileHandle_, 0, length & 0xFFFFFFFF, length >> 32, &overlapped);
}
*/

} // namespace clarisma