// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <vector>

namespace clarisma {

class Table 
{
public:
    class Column
    {
    private:
        uint32_t width_;
    };

    class Cell
    {
    private:
        const char* data_;
        uint32_t size_;
        uint32_t width_;
    };

private:
    std::vector<Column> columns_;
    std::vector<Cell> cells_;
};

} // namespace clarisma