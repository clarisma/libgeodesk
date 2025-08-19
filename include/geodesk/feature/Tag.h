// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <utility>
#include <geodesk/feature/TagValue.h>

namespace geodesk {

/// @brief A key/value pair that describes an attribute of a Feature.
///
/// Converts implicitly to `std::pair<K,V>`.
///
/// **Warning**: A Tag object is a lightweight wrapper
/// around a pointer to a Feature's tag data, contained
/// in a FeatureStore. It becomes invalid once that
/// FeatureStore is closed. To use a Feature's keys and
/// values beyond the lifetime of the FeatureStore,
/// convert them to `std::string`, which allocates
/// copies.
///
/// @see Tags, TagValue
///
class /* GEODESK_API */ Tag
{
public:
    Tag() = default;
    Tag(StringValue k, TagValue v) : key_(k), value_(v) {}

    /// @brief Returns this tag as a `std::pair`
    ///
    /// @tparam K  StringValue, `std::string` or `std::string_view`
    /// @tparam V  TagValue, `std::string`, `double`, `int` or `bool`
    ///
    template<typename K, typename V>
    // requires std::is_convertible_v<decltype(key_), K> && std::is_convertible_v<decltype(value_), V>
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::pair<K,V>() const noexcept
    {
        return {key_,value_};
    }

    /// @brief Checks if both Tag objects have the same key and value.
    ///
    bool operator==(const Tag& other) const
    {
        return key_ == other.key_ && value_ == other.value_;
    }

    bool operator!=(const Tag& other) const
    {
        return !(*this == other);
    }

    bool operator<(const Tag& other) const noexcept
    {
        return key_ < other.key_;
    }

    /// @brief The Tag's key.
    ///
    StringValue key() const noexcept { return key_; }

    /// @brief The Tag's value.
    ///
    TagValue value() const noexcept { return value_; }

private:
    StringValue key_;
    TagValue value_;
};

} // namespace geodesk
