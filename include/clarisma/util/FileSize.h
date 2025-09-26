// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/text/Format.h>
#include <clarisma/util/streamable.h> // for << operator support

// TODO: move to text

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

    template<typename Stream>
    void format(Stream& out) const
    {
        char buf[32];
        char* p = format(buf);
        out.write(buf, p - buf);
    }

private:
    uint64_t size_;
};

} // namespace clarisma
