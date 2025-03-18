// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <utility>
#include <geodesk/feature/TagValue.h>

namespace geodesk {

/// @brief A lightweight wrapper for a key string.
///
/// A Key is constructed from a `std::string_view` via Features::key()
/// and is valid for all features within the same GOL.
/// A Key created for one GOL cannot be used for lookups
/// in another.
///
/// A Key object is useful in scenarios requiring lookups of the
/// same tag across multiple features. It avoids the overhead
/// of resolving the tag's global-string code on every lookup,
/// leading to a performance boost of about 2x to 4x.
///
/// ```
/// Key houseNumber = buildings.key("addr:housenumber");
/// for(Feature building: buildings)
/// {
///     TagValue hn = building[houseNumber];
///     // This is faster than:
///     TagValue hnSlow = building["addr:housenumber"];
/// }
/// ```
///
/// Key converts implicitly to `std::string_view`.
///
/// @warning The memory backing the `std::string_view` used to create this
/// Key must remain unchanged and valid for the Key's entire lifetime
/// to avoid undefined behavior.
///
/// @see Tags, Feature
///
class /* GEODESK_API */ Key
{
public:
    /// Creates a null Key.
    ///
    /// Any lookup with a null Key returns an empty TagValue.
    /// Calling hasTag() with a null Key always returns `false`.
    /// 
    Key() : code_(0), size_(0), data_(nullptr) {}

    /// @fn operator std::string_view() const noexcept
    /// @brief The key as a `std::string_view` (implicit conversion)
    ///
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::string_view() const noexcept { return { data_, size_}; }

    /// @brief The character data of the key string
    ///
    /// **Note:** The string is *not* guaranteed to 0-terminated.
    ///
    const char* data() const noexcept { return data_; }

    /// @brief The size of the key string (in bytes, *not* characters).
    ///
    uint32_t size() const noexcept { return size_; }

    /// @brief The global-string code of this Key, or -1 if it
    /// is not in the global-string table.
    ///
    int code() const noexcept { return code_; }

    bool operator==(const Key&) const = default; // C++20
    bool operator!=(const Key&) const = default; // C++20

private:
    // Can only be constructed by a FeatureStore
    Key(const char* data, int32_t size, int code) :
        code_(code), size_(size), data_(data) {}

    int code_;
    uint32_t size_;
    const char* data_;

    friend class FeatureStore;
};

template<typename Stream>
Stream& operator<<(Stream& out, const Key& k)
{
    out.write(k.data(), k.size());
	return static_cast<Stream&>(out);
}

} // namespace geodesk
