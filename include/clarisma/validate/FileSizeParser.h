// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/util/Parser.h>

namespace clarisma {

class FileSizeParser : protected Parser
{
public:
    explicit FileSizeParser(const char* s) : Parser(s) {}

    uint64_t parse();
};

} // namespaceclarisma
