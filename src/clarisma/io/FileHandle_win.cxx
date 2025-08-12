// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/FileHandle.h>
#include <stdexcept>

namespace clarisma {

void File::makeSparse()
{
    DWORD bytesReturned = 0;
    if(!DeviceIoControl(
        fileHandle_,          // Handle to the file obtained with CreateFile
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

void File::allocate(uint64_t ofs, size_t length)
{
    // TODO: does not really exist on Windows
}

void File::deallocate(uint64_t ofs, size_t length)
{
    zeroFill(ofs, length);
}


void File::zeroFill(uint64_t ofs, size_t length)
{
    FILE_ZERO_DATA_INFORMATION zeroDataInfo;
    zeroDataInfo.FileOffset.QuadPart = ofs;
    zeroDataInfo.BeyondFinalZero.QuadPart = ofs + length;

    DWORD bytesReturned = 0;
    if (!DeviceIoControl(
        fileHandle_,                 // handle to file
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

uint64_t File::allocatedSize() const
{
    FILE_STANDARD_INFO fileInfo;
    if (!GetFileInformationByHandleEx(fileHandle_, FileStandardInfo,
        &fileInfo, sizeof(fileInfo)))
    {
        IOException::checkAndThrow();
    }
    return fileInfo.AllocationSize.QuadPart;
}

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