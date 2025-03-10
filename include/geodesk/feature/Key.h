// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <utility>
#include <geodesk/feature/TagValue.h>

namespace geodesk {

/// @brief A lightweight wrapper around a key string, enabling
/// fast lookup of tag values.
/// A Key is constructed from `std::string_view` via `Features::key()`
/// and is valid for all features that are stored in the same GOL.
/// A Key creates for Features in one GOL cannot be used for lookups
/// in another GOL.
///
/// Converts implicitly to `std::string_view`.
///
/// **Important**: The memory backing the `std::string_view` on which
/// this `Key` is based must not be changed or freed during the lifetime
/// of the `Key`, or its usage may result in undefined behavior.
///
/// @see Tags, TagValue
///
class /* GEODESK_API */ Key
{
public:
    /// @fn operator std::string_view() const noexcept
    /// @brief The key as a `std::string_view` (implicit conversion)
    ///
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::string_view() const noexcept { return { data_, size_}; }

    const char* data() const noexcept { return data_; }
    uint32_t size() const noexcept { return size_; }
    int code() const noexcept { return code_; }

    bool operator==(const Key&) const = default; // C++20
    bool operator!=(const Key&) const = default; // C++20

private:
    Key(const char* data, int32_t size, int code) :
        code_(code), size_(size), data_(data) {}

    int code_;
    uint32_t size_;
    const char* data_;

    friend class FeatureStore;
};

} // namespace geodesk
