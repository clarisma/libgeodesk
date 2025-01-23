// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <stdexcept>
#include <cstdint>
#include <clarisma/util/Bits.h>

namespace clarisma {

inline uint64_t safeReadVarint64(const uint8_t*& p, const uint8_t* end)
{
	uint64_t val = 0;
	int shift = 0;
	while(p < end)
	{
		uint8_t b = *p++;
		val |= (b & 0x7f) << shift;
		shift += 7;
		if ((b & 0x80) == 0) return val;
		if(shift > 63)
		{
			throw std::runtime_error("Invalid varint (more than 10 bytes)");
		}
	}
	throw std::runtime_error("Invalid varint (extends past end of buffer)");
}

inline uint32_t safeReadVarint32(const uint8_t*& p, const uint8_t* end)
{
	uint32_t val = 0;
	int shift = 0;
	while(p < end)
	{
		uint8_t b = *p++;
		val |= (b & 0x7f) << shift;
		shift += 7;
		if ((b & 0x80) == 0) return val;
		if(shift > 28)
		{
			throw std::runtime_error("Invalid varint (more than 5 bytes)");
		}
	}
	throw std::runtime_error("Invalid varint (extends past end of buffer)");
}

inline int32_t safeReadSignedVarint32(const uint8_t*& p, const uint8_t* end)
{
	int64_t val = static_cast<int64_t>(safeReadVarint64(p, end));
	return static_cast<int32_t>((val >> 1) ^ -(val & 1));
}

inline int64_t safeReadSignedVarint64(const uint8_t*& p, const uint8_t* end)
{
	int64_t val = static_cast<int64_t>(safeReadVarint64(p, end));
	return (val >> 1) ^ -(val & 1);
}

} // namespace clarisma
