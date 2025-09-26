// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdint>
#include <string_view>
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    #include <emmintrin.h>  // Header for SSE2 intrinsics
#endif

namespace clarisma {

namespace Math
{
/**
 * Parses a double value, ignoring any additional non-numeric text.
 * Scientific notation is not supported.
 *
 * @param s		   pointer to the string
 * @param len      length of the string
 * @param pResult  where to store the parsed double
 * @returns true if successful, else false if the start of the string
 *    does not contain a valid number
 */
extern bool parseDouble(const char* s, size_t len, double* pResult);
inline bool parseDouble(std::string_view s, double* pResult)
{
	return parseDouble(s.data(), s.length(), pResult);
}

/**
 * Returns average of two int32_t values, ensuring that potential
 * overflow is handled correctly.
 */
inline int32_t avg(int32_t a, int32_t b)
{
	return static_cast<int32_t>((static_cast<int64_t>(a) + b) / 2);
}

/// Fast standard rounding and truncating to int32, without support
/// for NaN and infinity. Uses intrinsics if supported.
///
inline int32_t roundFastToInt32_intrin(double x)
{
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    // Use SSE2 intrinsics if available
    // Fast SSE2 rounding to nearest, ties to even
    return _mm_cvtsd_si32(_mm_set_sd(x));
#else
    // Portable fallback: round to nearest, ties away from zero
    return static_cast<int32_t>(x + (x >= 0.0 ? 0.5 : -0.5));
#endif
}

inline int32_t roundFastToInt32(double x)
{
    return static_cast<int32_t>(x + (x >= 0.0 ? 0.5 : -0.5));
}

extern double POWERS_OF_10[];
}

} // namespace clarisma
