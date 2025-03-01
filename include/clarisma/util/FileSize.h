// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/text/Format.h>

namespace clarisma {

class FileSize 
{
public:
    explicit FileSize(uint64_t size) : size_(size) {}
    operator uint64_t() const noexcept { return size_; }  // NOLINT implicit ok

    char* format(char* buf) const
    {
        return format(buf, size_);
    }

    static char* format(char* buf, uint64_t size)
    {
        return Format::fileSizeNice(buf, size);
    }

private:
    uint64_t size_;
};

template<typename Stream>
Stream& operator<<(Stream& out, FileSize size)
{
    char buf[32];
    size.format(buf);
    std::string_view sv = buf;
    out.write(sv.data(), sv.size());
    return static_cast<Stream&>(out);
}

} // namespace clarisma
