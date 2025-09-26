// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <cassert>
#include <span>
#include <string_view>
#include <vector>
#include "TextMetrics.h"

namespace clarisma {

class BufferWriter;

class Table 
{
public:
    class Column
    {
    public:
        int width() const { return width_; };
        void setWidth(int width) { width_ = width; }

    private:
        int width_;
    };

    class Cell
    {
    public:
        Cell() : data_(nullptr), size_(0), width_(0) {};

        Cell(std::string_view s) :
            data_(s.data()),
            size_(static_cast<int>(s.size())),
            width_(static_cast<int>(TextMetrics::countCharsUtf8(s))) {}

        Cell(std::string_view s, uint32_t width) :
            data_(s.data()),
            size_(static_cast<int>(s.size())),
            width_(width) {}

        const char* data() const { return data_; };
        int size() const { return size_; }
        int width() const { return width_; }

    private:
        const char* data_;
        int size_;
        int width_;
    };

    Table();

    int colCount() const { return static_cast<int>(columns_.size()); }
    int rowCount() const { return static_cast<int>(cells_.size() / colCount()); }    // TODO

    std::span<Cell> operator[](size_t row)
    {
        size_t colCount = columns_.size();
        size_t n = row * columns_.size();
        assert(n + colCount <= cells_.size());
        return { &cells_[n], colCount };
    }

    const std::span<const Cell> operator[](size_t row) const
    {
        size_t colCount = columns_.size();
        size_t n = row * columns_.size();
        assert(n + colCount <= cells_.size());
        return { &cells_[n], colCount };
    }

    void distributeColumns(std::span<const Cell> cells, int maxCols, int maxWidth);
    void writeTo(BufferWriter& out, int indent = 0) const;
    static void writeTo(BufferWriter& out, const Column& col, const Cell& cell);

private:
    std::vector<Column> columns_;
    std::vector<Cell> cells_;
};

} // namespace clarisma