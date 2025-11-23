// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <geodesk/geom/Coordinate.h>

// \cond

namespace geodesk {

class CoordinateSpanIterator
{
public:
  	template<typename Container>
	CoordinateSpanIterator(const Container& container) :
		CoordinateSpanIterator(container.data(), container.size()) {}

    CoordinateSpanIterator(const Coordinate* coords, size_t count) :
		p_(coords),
		remaining_(count)
	{
	}

	int coordinatesRemaining() const { return remaining_; }

	Coordinate next()
	{
        assert(remaining_ > 0);
		remaining_--;
        return *p_++;
	}

private:
	const Coordinate* p_;
    size_t remaining_;
};

} // namespace geodesk

// \endcond
