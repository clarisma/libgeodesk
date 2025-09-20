// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "File.h"
#include <clarisma/util/Strings.h>

namespace clarisma {

class AtomicFile : public File
{
public:
    AtomicFile() = default;
    AtomicFile(const AtomicFile&) = delete;
    AtomicFile& operator=(const AtomicFile&) = delete;
    AtomicFile(AtomicFile&&) = default;
    AtomicFile& operator=(AtomicFile&&) = default;

    ~AtomicFile()
    {
        if (isOpen()) tryClose();
    }

    // cannot be noexcept because it allocates a string
    bool tryOpen(const char* fileName, bool overwrite = false)
    {
        if (!overwrite)
        {
            File target;
            if (!target.tryOpen(fileName, OpenMode::CREATE | OpenMode::WRITE))
            {
                return false;
            }
            target.close();
        }
        fileName_ = fileName;
        tmpFileName_ = Strings::combine(fileName, ".tmp");
        return File::tryOpen(tmpFileName_.c_str(), OpenMode::CREATE | OpenMode::WRITE);
    }

    void open(const char* fileName, bool overwrite = false)
    {
        if (!tryOpen(fileName, overwrite)) throw IOException();
    }

    bool tryClose() noexcept
    {
        File::tryClose();
        return tryRename(tmpFileName_.c_str(), fileName_);
    }

    void close()
    {
        if (!tryClose()) throw IOException();
    }

private:
    const char *fileName_ = nullptr;
    std::string tmpFileName_;
};


} // namespace clarisma
