// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/FileHandle.h>
#include <winioctl.h>   // For FSCTL_SET_SPARSE

namespace clarisma {

void FileHandle::open(const char* fileName, OpenMode mode)
{
    if (!tryOpen(fileName, mode))
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            throw FileNotFoundException(fileName);
        }
        throw IOException();
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
        throw IOException();
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
        throw IOException();
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


std::string FileHandle::errorMessage()
{
    LPSTR buffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
        static_cast<DWORD>(error()),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buffer, 0, NULL);
    std::string message(buffer, size);
    LocalFree(buffer);
    return message;
}


std::string FileHandle::errorMessage(const char* fileName)
{
    LPSTR buffer = nullptr;
    size_t msgLen = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
        static_cast<DWORD>(error()),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buffer, 0, NULL);

    size_t fileNameLen = std::strlen(fileName);
    std::string s;
    s.reserve(fileNameLen + 2 + msgLen);
    s.append(fileName, fileNameLen);
    s.append(": ", 2);
    s.append(buffer, msgLen);
    LocalFree(buffer);
    return s;
}

/*
void FileHandle::formatError(char* buf, int bufSize)
{
    int errorCode = getL
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buf, bufSize, NULL);
}
*/

} // namespace clarisma