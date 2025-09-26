// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/util/DateTime.h>

namespace clarisma {

class FileTime
{
public:
    explicit FileTime(const char* path);

    DateTime created() const { return created_; }
    DateTime modified() const { return modified_; }
    DateTime accessed() const { return accessed_; }

private:
    DateTime created_;
    DateTime modified_;
    DateTime accessed_;
};

} // namespace clarisma
