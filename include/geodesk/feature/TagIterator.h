// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/TagTablePtr.h>

namespace geodesk {

/// \cond lowlevel

class GEODESK_API TagIterator
{
public:
	TagIterator(TagTablePtr tags, StringTable& strings);

	std::pair<const clarisma::ShortVarString*, TagBits> next();
	TagTablePtr tags() const { return tags_; }
	StringTable& strings() const { return strings_; }

private:
	TagTablePtr tags_;
	DataPtr p_;
	StringTable& strings_;
};

// \endcond
} // namespace geodesk
