// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/File.h>
#include <limits.h> // for PATH_MAX
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h> // for off_t
#include <unistd.h>
#include <fcntl.h>

namespace clarisma {

std::string FileHandle::fileName() const
{
    char fdPath[1024];
    char filePath[1024];
    snprintf(fdPath, sizeof(fdPath), "/proc/self/fd/%d", fileHandle_);
    ssize_t len = readlink(fdPath, filePath, sizeof(filePath) - 1);
    if (len != -1)
    {
        filePath[len] = '\0'; // Null-terminate the result
        return std::string(filePath);
    }
    return "<invalid file>";
}


void File::allocate(uint64_t ofs, size_t length)
{
#ifdef __APPLE__
    // TODO: no native implementation of fallocate() on MacOS,
    //  do nothing for now
#else
    // Could use posix_fallocate on Linux as well, buf fallocate is more efficient
    if (fallocate(fileHandle_, 0, ofs, length) != 0)
    {
        IOException::checkAndThrow();
    }
#endif
}

void File::deallocate(uint64_t ofs, size_t length)
{
    // TODO: do nothing for now
}


void File::zeroFill(uint64_t ofs, size_t length)
{
    // TODO: do nothing for now
}


bool File::exists(const char* fileName)
{
    struct stat buffer;
    if (stat(fileName, &buffer) != 0)
    {
        if(errno != ENOENT) IOException::checkAndThrow();
        return false;
    }
    return true;
}

void File::remove(const char* fileName)
{
    if (unlink(fileName) != 0)
    {
        IOException::checkAndThrow();
    }
}

void File::rename(const char* from, const char* to)
{
    if(std::rename(from, to) != 0) IOException::checkAndThrow();
}

std::string File::path(int handle)
{
#if defined(__linux__)
    // Linux implementation
    char buf[PATH_MAX];
    std::string fdPath = "/proc/self/fd/" + std::to_string(handle);
    ssize_t res = readlink(fdPath.c_str(), buf, PATH_MAX - 1);
    if (res == -1)
    {
        IOException::checkAndThrow();
        return "";
    }
    buf[res] = '\0'; // Null-terminate the result
    return std::string(buf);
#elif defined(__APPLE__)
    // macOS implementation
    char buf[PATH_MAX];
    if (fcntl(handle, F_GETPATH, buf) == -1)
    {
        IOException::checkAndThrow();
        return "";
    }
    return std::string(buf);
#else
    #error "Unsupported platform for File::path implementation"
#endif
}

uint64_t File::allocatedSize() const
{
    struct stat fileStat;
    if (fstat(fileHandle_, &fileStat) != 0)
    {
        IOException::checkAndThrow();
    }
    return fileStat.st_blocks * 512; // Convert blocks to bytes

    // Linux/macOS, st_blocks in struct stat always reports blocks
    // in 512-byte units, even if the actual filesystem block size is larger
    // TODO: verify if true
}

bool File::tryLock(uint64_t ofs, uint64_t length, bool shared)
{
    struct flock fl;
    fl.l_type = shared ? F_RDLCK : F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = ofs;
    fl.l_len = length;
    return fcntl(fileHandle_, F_SETLK, &fl) >= 0;
}


bool File::tryUnlock(uint64_t ofs, uint64_t length)
{
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = ofs;
    fl.l_len = length;
    return fcntl(fileHandle_, F_SETLK, &fl) >= 0;
}


} // namespace clarisma