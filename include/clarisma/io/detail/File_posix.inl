// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

// POSIX inline implementations for clarisma::FileHandle

#include <cstddef>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>       // read, write, pread, pwrite, ftruncate, fsync
#include <sys/mman.h>     // mmap
#include <sys/stat.h>     // fstat
#include <sys/types.h>    // off_t
#include <clarisma/io/IOException.h>

static_assert(sizeof(off_t) >= 8, "off_t must be 64-bit");
static_assert(sizeof(ssize_t) >= 8, "ssize_t must be 64-bit");

namespace clarisma
{

inline bool File::tryRename(const char* from, const char* to)
{
    return std::rename(from, to) == 0;
}

inline void File::rename(const char* from, const char* to)
{
    if(!tryRename(from, to)) IOException::checkAndThrow();
}

} // namespace clarisma
