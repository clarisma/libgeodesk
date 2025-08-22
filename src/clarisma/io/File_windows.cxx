// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/File.h>
#include <stdexcept>

namespace clarisma {

/*
void File::open(const char* filename, int mode)
{
    DWORD access = 0;
    DWORD creationDisposition = 0;

    if (mode & OpenMode::READ)
    {
        access |= GENERIC_READ;
    }
    if (mode & OpenMode::WRITE)
    {
        access |= GENERIC_WRITE;
    }

    if (mode & REPLACE_EXISTING)
    {
        creationDisposition = CREATE_ALWAYS;
    }
    else if (mode & OpenMode::CREATE)
    {
        creationDisposition = OPEN_ALWAYS;
    }
    else
    {
        creationDisposition = OPEN_EXISTING;
    }

    // TODO: set a share mode instead of `0`
    //  Should use (FILE_SHARE_READ | FILE_SHARE_WRITE) unless file is 
    //  expressively opened as exclusive? or other way around, make shared access explicit?
    //  (Linux does not have exclusive access mode, multiple processes can read/write)
    // For now, we enable shared access implicitly to match Linux behavior
    fileHandle_ = CreateFileA(filename, access, FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL, creationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fileHandle_ == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            throw FileNotFoundException(filename);
        }
        IOException::checkAndThrow();
    }

    // TODO: only do this if file did not exist
    if (mode & SPARSE)
    {
        DWORD bytesReturned;
        DeviceIoControl(fileHandle_, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &bytesReturned, NULL);
    }
}

void File::close()
{
    if (fileHandle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(fileHandle_);
        fileHandle_ = INVALID_HANDLE_VALUE;
    }
}


uint64_t File::size() const
{
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle_, &fileSize))
    {
        IOException::checkAndThrow();
    }
    return fileSize.QuadPart;
}

void File::setSize(uint64_t newSize)
{
    LARGE_INTEGER newPos;
    newPos.QuadPart = newSize;
    if (!SetFilePointerEx(fileHandle_, newPos, NULL, FILE_BEGIN))
    {
        IOException::checkAndThrow();
    }
    if (!SetEndOfFile(fileHandle_))
    {
        IOException::checkAndThrow();
    }
}

void File::expand(uint64_t newSize)
{
    if (size() < newSize)
    {
        setSize(newSize);
    }
}

void File::truncate(uint64_t newSize)
{
    if (size() > newSize)
    {
        setSize(newSize);
    }
}

void File::force()
{
    if (!FlushFileBuffers(fileHandle_)) 
    {
        IOException::checkAndThrow();
    }
}


void File::seek(uint64_t posAbsolute)
{
    DWORD dwPtrLow = posAbsolute & 0xFFFFFFFF;
    LONG  dwPtrHigh = (posAbsolute >> 32) & 0xFFFFFFFF;
    DWORD resultLow = SetFilePointer(fileHandle_, dwPtrLow, &dwPtrHigh, FILE_BEGIN);
    if (resultLow == INVALID_SET_FILE_POINTER) // && GetLastError() != NO_ERROR)
    {
        // Have to also check for error, since INVALID_SET_FILE_POINTER
        // could also be part of a valid position - -however, checkAndThrow
        // does just that
        IOException::checkAndThrow();
    }
}


size_t File::read(void* buf, size_t length)
{
    DWORD bytesRead;
    if (!ReadFile(fileHandle_, buf, static_cast<DWORD>(length), &bytesRead, NULL))
    {
        IOException::checkAndThrow();
    }
    return bytesRead;
}


size_t File::read(uint64_t ofs, void* buf, size_t length)
{
    // BOOL ReadFromFile(HANDLE hFile, LPVOID buf, DWORD length, DWORD64 ofs)
    
    OVERLAPPED overlapped = { 0 };
    overlapped.Offset = (DWORD)(ofs & 0xFFFFFFFF);
    overlapped.OffsetHigh = (DWORD)(ofs >> 32);
    DWORD bytesRead;
    if (!ReadFile(fileHandle_, buf, static_cast<DWORD>(length), &bytesRead, &overlapped))
    {
        IOException::checkAndThrow();
    }
    return bytesRead;
}


size_t File::write(const void* buf, size_t length)
{
    DWORD bytesWritten;
    assert(length <= MAXDWORD);

    if (!WriteFile(fileHandle_, buf, static_cast<DWORD>(length), &bytesWritten, nullptr))
    {
        IOException::checkAndThrow();
    }
    return bytesWritten;
}

size_t File::tryWriteAt(uint64_t ofs, const void* buf, size_t length)
{
    DWORD bytesWritten;
    OVERLAPPED ovl;
    ovl.Offset     = static_cast<DWORD>(ofs & 0xFFFFFFFFull);
    ovl.OffsetHigh = static_cast<DWORD>((ofs >> 32) & 0xFFFFFFFFull);
    bool res = WriteFile(fileHandle_, buf, static_cast<DWORD>(length),
        &bytesWritten, &ovl);
    return res ? bytesWritten : 0;
}
*/

/*

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

*/

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

/*
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