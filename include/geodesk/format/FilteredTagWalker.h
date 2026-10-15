// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/TagWalker.h>
#include <geodesk/format/KeySchema.h>

namespace geodesk {

/// \cond lowlevel

// Alternative to TagIterator

class FilteredTagWalker : public TagWalker
{
public:
	FilteredTagWalker(TagTablePtr tags, StringTable& strings, const KeySchema* schema) :
		TagWalker(tags, strings),
		schema_(schema)
  	{
    }

	int next()
	{
		while(TagWalker::next())
        {
			if(!schema_) return -1;
			if(keyCode() >= 0) [[likely]]
			{
				int column = schema_->columnOfGlobal(keyCode());
			    if(column) return column;
			}
			else
			{
            	int column = schema_->columnOfLocal(key()->toStringView());
            	if(column) return column;
            }
		}
		return 0;
    }

private:
	const KeySchema* schema_;
};

// \endcond
} // namespace geodesk
