// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <cstdint>
#include <string_view>
#include <clarisma/util/Bits.h>

namespace clarisma {

inline uint64_t readVarint35(const uint8_t*& p)
{
	uint32_t val;
	uint8_t b;
	b = *p++;
	val = b & 0x7f;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint32_t>(b & 0x7f) << 7;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint32_t>(b & 0x7f) << 14;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint32_t>(b & 0x7f) << 21;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint32_t>(b & 0x7f) << 28;
	assert((b & 0x80) == 0);
	return val;
}

inline uint32_t readVarint32(const uint8_t*& p)
{
	return static_cast<uint32_t>(readVarint35(p));
}

// TODO: Read 9 bytes max, last byte does not need a continuation bit
//  8 x 7 bits = 56 bits, plus final 8 bits means we can store 64 bits
//  in 9 bytes

inline uint64_t readVarint64(const uint8_t*& p)
{
	uint64_t val;
	uint8_t b;
	b = *p++;
	val = b & 0x7f;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint64_t>(b & 0x7f) << 7;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint64_t>(b & 0x7f) << 14;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint64_t>(b & 0x7f) << 21;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint64_t>(b & 0x7f) << 28;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint64_t>(b & 0x7f) << 35;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint64_t>(b & 0x7f) << 42;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint64_t>(b & 0x7f) << 49;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint64_t>(b & 0x7f) << 56;
	if ((b & 0x80) == 0) return val;
	b = *p++;
	val |= static_cast<uint64_t>(b & 0x7f) << 63;
	assert((b & 0x80) == 0);
	return val;
}

inline int64_t readSignedVarint35(const uint8_t*& p)
{
	int64_t val = static_cast<int64_t>(readVarint35(p));
	return (val >> 1) ^ -(val & 1);
}

inline int32_t readSignedVarint32(const uint8_t*& p)
{
	int64_t val = static_cast<int64_t>(readVarint35(p));
	return static_cast<int32_t>((val >> 1) ^ -(val & 1));
}

inline int64_t readSignedVarint64(const uint8_t*& p)
{
	int64_t val = static_cast<int64_t>(readVarint64(p));
	return (val >> 1) ^ -(val & 1);
}


inline std::string_view readStringView(const uint8_t*& p)
{
	uint32_t len = readVarint32(p);
	std::string_view sv(reinterpret_cast<const char*>(p), len);
	p += len;
	return sv;
}

inline int countVarints(const void* pStart, const void* pEnd)
{
	int count = 0;
	const char* p = reinterpret_cast<const char*>(pStart);
	while (p < pEnd)
	{
		if (*p++ >= 0) count++;
	}
	return count;
}

inline void skipVarints(const uint8_t*& p, int count)
{
	do
	{
		uint8_t b = *p++;
		count -= (b >> 7) ^ 1;
	}
	while (count);
}


/// Moves pointer backward, skipping over the specified
/// numbers of varints. This function must guarantee the
/// following:
/// - p is placed right after the last varint
/// - a valid varint must precede the varint to which to skip
///
inline void skipVarintsBackwardUnsafe(const uint8_t*& p, int count)
{
	do
	{
		p--;
		uint8_t b = *(p-1);
		count -= (b >> 7) ^ 1;
	}
	while (count);
}

// TODO: modify so we can write 64 bits in 9 bytes, no continuation bit in final
//  No, that would not comply with LEB128

inline void writeVarint(uint8_t*& p, uint64_t val)
{
	while (val >= 0x80)
	{
		*p++ = (val & 0x7f) | 0x80;
		val >>= 7;
	}
	*p++ = static_cast<uint8_t>(val);
}


inline void writeSignedVarint(uint8_t*& p, int64_t val)
{
	writeVarint(p, (val << 1) ^ (val >> 63));
}

/**
 * Returns the number of bytes required to encode the given 
 * unsigned value as a varint (A varint requires one byte for
 * each complete or partial run of 7 significant bits)
 */
inline unsigned int varintSize(uint64_t v)
{
	return (64 - Bits::countLeadingZerosInNonZero64(v | 1) + 6) / 7;
}


inline uint64_t toZigzag(int64_t v) 
{
	return (v << 1) ^ (v >> 63);
}

inline uint32_t toZigzag(int32_t v)
{
	return (v << 1) ^ (v >> 31);
}

inline int64_t fromZigzag(uint64_t v) 
{
	return static_cast<int64_t>((v >> 1) ^ -static_cast<int64_t>(v & 1));
}

inline int32_t fromZigzag(uint32_t v)
{
	return static_cast<int32_t>((v >> 1) ^ -static_cast<int32_t>(v & 1));
}

/*

class Varint
{
public:
    explicit Varint(uint64_t value) : value_(value) {}
    uint64_t value() const { return value_; }

private:
    uint64_t value_;
};

class SignedVarint
{
public:
	explicit SignedVarint(int64_t value) : value_(value) {}
	int64_t value() const { return value_; }

private:
	int64_t value_;
};

template<typename Stream>
Stream& operator<<(Stream& out, Varint v)
{
	uint8_t buf[16];
	uint8_t* p = buf;
	writeVarint(p, v.value());
	out.write(reinterpret_cast<const char*>(buf), p-buf);
	return static_cast<Stream&>(out);
}

template<typename Stream>
Stream& operator<<(Stream& out, SignedVarint v)
{
	uint8_t buf[16];
	uint8_t* p = buf;
	writeSignedVarint(p, v.value());
	out.write(reinterpret_cast<const char*>(buf), p-buf);
	return static_cast<Stream&>(out);
}

*/



} // namespace clarisma
