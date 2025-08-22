// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <limits>
#include <clarisma/text/Format.h>
#include <clarisma/util/streamable.h> // for << operator support
#include <geodesk/geom/Box.h>

namespace clarisma
{
class BufferWriter;
}

namespace geodesk {

typedef int32_t ZoomLevel;
using clarisma::operator<<;

/// \cond lowlevel

class Tile
{
public:
	constexpr Tile() : tile_(EMPTY) {}
	constexpr explicit Tile(uint32_t v) : tile_(v) {}
	constexpr Tile(const Tile& other) = default;

	bool isNull() const
	{
		return tile_ == EMPTY;
	}

	Tile& operator=(const Tile& other) = default;

	explicit operator uint32_t() const
	{
		return tile_;
	}

	bool operator==(const Tile& other) const 
	{
		return tile_ == other.tile_;
	}

	static constexpr int columnFromXZ(int32_t x, ZoomLevel zoom)
	{
		return (int)((static_cast<long long>(x) + (1LL << 31)) >> (32 - zoom));
	}

	static constexpr int rowFromYZ(int32_t y, ZoomLevel zoom)
	{
		return (int)((0x7fffffffLL - y) >> (32 - zoom));
	}

	int row() const
	{
		return static_cast<int>((tile_ >> 12) & 0xfff);
	}

	int column() const
	{
		return static_cast<int>(tile_ & 0xfff);
	}

	int zoom() const
	{
		return static_cast<int>((tile_ >> 24) & 0xf);
	}

	static Tile fromString(const char* s);

	static constexpr Tile fromColumnRowZoom(int col, int row, ZoomLevel zoom)
	{
		return Tile(col, row, zoom);
	}

	// TODO: only works for positive deltas, and does not wrap!
	Tile relative(int deltaCol, int deltaRow) const
	{
		assert(deltaCol >= 0);
		assert(deltaRow >= 0);
		return Tile(tile_ + (deltaRow << 12) + deltaCol);
	}

	int topY() const
	{
		return std::numeric_limits<int32_t>::max() - (row() << (32 - zoom()));
	}

	int bottomY() const
	{
		return std::numeric_limits<int32_t>::min() - (int)((int64_t)(row() + 1) << (32 - zoom()));
		// << 32 wraps around for int, that's why we cast to long
	}

	int leftX() const
	{
		int z = zoom();
		int col = column();
		return (col - (1 << (z - 1))) << (32 - z);
	}

	int rightX() const
	{
		int64_t extent = 1LL << (32 - zoom());
		return static_cast<int>(leftX() + extent - 1);
	}

	Box bounds() const
	{
		int z = zoom();
		int minX = leftX();
		int minY = bottomY();
		int64_t extent = 1LL << (32 - z);
		return Box(minX, minY, (int)(minX + extent - 1), (int)(minY + extent - 1));
	}

	/**
	 * Returns the tile number of an adjacent tile that lies
	 * in the specified direction.
	 *
     * @param  colDelta
     * @param  rowDelta
     * @return the tile number of the adjacent tile
     */
	Tile neighbor(int colDelta, int rowDelta) const
	{
		int z = zoom();
		int x = column();
		int y = row();
		int mask = (1 << z) - 1;
		x = (x + colDelta) & mask;
		y = (y + rowDelta) & mask;
		return fromColumnRowZoom(x, y, z);
	}

	std::string toString() const
	{
		char buf[64];
		buf[sizeof(buf) - 1] = '\0';
		char* p = formatReverse(&buf[sizeof(buf) - 1]);
		return {p};
	}

	char* formatReverse(char* end) const;

	template<typename S>
	void format(S& out) const
	{
		char buf[64];
		char* end = &buf[sizeof(buf)];
		char* start = formatReverse(end);
		out.write(start, end - start);
	}

	/**
	 * Returns the tile that contains this tile at the specified
	 * (lower) zoom level. If the zoom level is the same, the
	 * tile itself is returned.
	 *
	 * @param lowerZoom	zoom level of the parent tile
	 *              (must be <= the tile's zoom level)
	 * @return	the lower-zoom tile that contains the tile
	 */
	Tile zoomedOut(const int lowerZoom) const
	{
		const int currentZoom = this->zoom();
		assert(lowerZoom <= currentZoom); // Can't zoom out to higher level
		const int delta = currentZoom - lowerZoom;
		return fromColumnRowZoom(column() >> delta, row() >> delta, lowerZoom);
	}

	enum Twin
	{
		SELF = 0,
		NORTH_TWIN = 1,
		WEST_TWIN = 2,
		SOUTH_TWIN = 3,
		EAST_TWIN = 4,
		INVALID_TWIN = 7
	};

	uint_fast8_t isTwinOf(Tile other) const
	{
		assert(zoom() == other.zoom());
		uint_fast32_t colDelta = static_cast<uint_fast32_t>(column() - other.column() + 1);
		uint_fast32_t rowDelta = static_cast<uint_fast32_t>(row() - other.row() + 1);
		if (colDelta > 2 || rowDelta > 2) return Twin::INVALID_TWIN;

		// Delta is 0 (tile lies west/north), 1 (center) or 2 (tile lies east/south)
		// We combine the bits into a nibble (colDelta in lower, rowDelta in upper half)
		// We use the nibble to look up the twin code

		//                                  98'7654'3210
		constexpr uint64_t twins = 0x7777'7737'7402'7717;
			// 0101 (5) = center/self (0)
			// 0001 (1) = N (1)
			// 0100 (4) = W (2)
			// 1001 (9) = S (3)
			// 0110 (6) = E (4)
			// all others are invalid (7)
		
		uint_fast32_t shift = (rowDelta << 4) | (colDelta << 2);
			// already x4 to address the nibble within twins
		uint_fast8_t twinCode = static_cast<uint_fast8_t>((twins >> shift) & 15);
		// assert(twinCode == Twin::INVALID_TWIN || other.twin(twinCode) == *this);
		return twinCode;
	}

	Tile twin(uint_fast8_t twinCode) const
	{
		assert(twinCode >= 0 && twinCode <= 4);

		//                                4    3    2    1    0      
		constexpr uint32_t offsets = 0b0110'1001'0100'0001'0101;
		uint_fast32_t nibble = (offsets >> (twinCode * 4)) & 15;
		int colDelta = static_cast<int>(nibble & 3) - 1;
		int rowDelta = static_cast<int>(nibble >> 2) - 1;
		Tile twin = fromColumnRowZoom(
			column() + colDelta, row() + rowDelta, zoom());
		assert(twin.isTwinOf(*this) == twinCode);
		return twin;
	}

protected:
	static constexpr uint32_t EMPTY = 0xffff'ffff;

	constexpr Tile(int col, int row, ZoomLevel zoom)
		: tile_((zoom << 24) | (row << 12) | col) {}
	
	explicit Tile(int t) : tile_(t) {}
	
	uint32_t tile_;
};

} // namespace geodesk

template<>
struct std::hash<geodesk::Tile>
{
	size_t operator()(const geodesk::Tile& tile) const noexcept
	{
		return std::hash<uint32_t>()(static_cast<uint32_t>(tile));
	}
};

// \endcond

