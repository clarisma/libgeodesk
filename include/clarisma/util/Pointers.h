// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace clarisma {

namespace Pointers
{

inline int32_t delta32(const void* dest, const void* source)
{
	std::ptrdiff_t d = reinterpret_cast<const uint8_t*>(dest) - reinterpret_cast<const uint8_t*>(source);
	assert(d == static_cast<int32_t>(d));
	return static_cast<int32_t>(d);
}

inline uint32_t offset32(const void* dest, const void* source)
{
	assert(dest >= source);
    std::ptrdiff_t d = reinterpret_cast<const uint8_t*>(dest) - reinterpret_cast<const uint8_t*>(source);
	assert(d == static_cast<uint32_t>(d));
	return static_cast<uint32_t>(d);
}

inline uint64_t extractBits(const void *p, uint64_t mask)
{
	return reinterpret_cast<uintptr_t>(p) & mask;
}

template <typename T>
T* masked(T *p, uint64_t mask)
{
	return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) & mask);
}

}

} // namespace clarisma
