// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/text/Table.h>
#include <clarisma/util/StreamWriter.h>

namespace clarisma {

Table::Table()
{
}

void Table::distributeColumns(std::span<const Cell> data, int maxCols, int maxWidth)
{
    int gapWidth = 3;

    int colCount = maxCols;
    int rowCount;

    for(; colCount > 0; colCount--)
    {
        columns_.resize(colCount);
        rowCount = (data.size() + colCount - 1) / colCount;
        int tableWidth = 0;
        int colWidth = 0;
        int col = 0;
        int row = 0;
        for(auto item: data)
        {
            colWidth = std::max(colWidth, item.width());
            row++;
            if(row == rowCount)
            {
                tableWidth += (col ? gapWidth : 0) + colWidth;
                if(tableWidth > maxWidth) break;
                columns_[col++].setWidth(colWidth);
                row = 0;
                colWidth = 0;
            }
        }
        if(tableWidth <= maxWidth) break;
    }

    cells_.resize(colCount * rowCount);
    int row = 0;
    int col = 0;
    for(auto item : data)
    {
        (*this)[row][col] = item;
        row++;
        if(row == rowCount)
        {
            row = 0;
            col++;
        }
    }
}

void Table::writeTo(Buffer& out, const Column& col, const Cell& cell)
{
    int padding = col.width() - cell.width();
    out.write(cell.data(), cell.size());
    out.writeRepeatedChar(' ', padding);
}

void Table::writeTo(Buffer& out, int indent) const
{
    int col = 0;
    for(const Cell& cell : cells_)
    {
        if(col == 0)
        {
            out.writeRepeatedChar(' ', indent);
        }
        else
        {
            out << " | ";
        }
        writeTo(out, columns_[col], cell);
        col++;
        if(col == colCount())
        {
            out.writeByte('\n');
            col = 0;
        }
    }
}

} // namespace clarisma