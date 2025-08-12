// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#if defined(_WIN32)
#include "File_windows.cxx"
#elif defined(__linux__) || defined(__APPLE__) 
#include "File_linux.cxx"
#else
#error "Platform not supported"
#endif

namespace clarisma {

ByteBlock File::readBlock(size_t length)
{
    ByteBlock block(length);
    size_t bytesRead = readAll(block.data(), block.size());
    assert(bytesRead == length);
    return block;
}

ByteBlock File::readAll(const char* filename)
{
    File file;
    file.open(filename, OpenMode::READ);
    uint64_t size = file.getSize();
    return { file.readAll(size), size };
}


std::string File::readString(const char* filename)
{
    File file;
    file.open(filename, OpenMode::READ);
    uint64_t size = file.getSize();
    std::string s;
    s.resize(size+1);
    file.readAll(&s[0], size);
    s[size] = '\0';
    return s;
}


void File::writeAll(const char* filename, const void* data, size_t size)
{
    File file;
    file.open(filename, OpenMode::WRITE | OpenMode::CREATE | OpenMode::REPLACE_EXISTING);
    file.writeAll(data, size);
}

} // namespace clarisma
