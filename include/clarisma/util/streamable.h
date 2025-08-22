// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace clarisma {

class Buffer;

/// @brief True if S is not Buffer-derived by static type.
template<typename S>
concept NotBufferLike =
    !std::is_base_of_v<Buffer, std::remove_reference_t<S>>;

/// @brief Writer that has a usable write(); supports ostream- and
///        buffer-style signatures without including <ostream>.
template<typename S>
concept StreamLike =
    requires (S& s)
{
    // std::ostream-like: write(const char*, streamsize)
    s.write(static_cast<const char*>(nullptr), 0);
} ||
requires (S& s)
{
    // byte-sink-like: write(const void*, size_t)
    s.write(static_cast<const void*>(nullptr), std::size_t{});
};

/// @brief T provides: template<class S> void format(S&) const
template<typename T, typename S>
concept FormatsWith =
    requires (const T& t, S& s)
{
    { t.template format<S>(s) } -> std::same_as<void>;
};

/// @brief Generic streaming for any non-Buffer stream that T can format to.
/// @details Include this in headers like Decimal.h to enable
///          `std::ostream << Decimal` via ADL, without including Buffer.
template<typename Stream, typename T>
    requires NotBufferLike<Stream> && StreamLike<Stream> &&
             FormatsWith<T, Stream>
Stream& operator<<(Stream& s, const T& value)
{
    value.template format<Stream>(s);
    return s;
}

} // namespace clarisma
