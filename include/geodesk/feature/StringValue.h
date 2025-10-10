// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/util/ShortVarString.h>
#include <clarisma/util/Strings.h>
#include <clarisma/util/streamable.h> // for << operator support
#include <geodesk/export.h>

namespace geodesk {

using clarisma::operator<<;

/// @brief A string pointer (used for keys, values and roles).
/// Character data is encoded as UTF-8.
///
/// Converts implicitly to `std::string` and `std::string_view`.
///
/// **Warning**: A StringValue object is a lightweight wrapper
/// around a pointer to string data, contained in a FeatureStore.
/// It becomes invalid once that FeatureStore
/// is closed. To use its value beyond the lifetime of its
/// FeatureStore, assign it to a `std::string` (which
/// allocates its own copy of the string).
///
class GEODESK_API StringValue
{
public:
    StringValue() : str_(clarisma::ShortVarString::empty()) {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    StringValue(const clarisma::ShortVarString* str) : str_(str) {}
    explicit StringValue(const uint8_t* bytes) :
        str_(reinterpret_cast<const clarisma::ShortVarString*>(bytes)) {}

    /// Pointer to the characters of the string.
    ///
    /// @return Pointer to the string's bytes (UTF-8 encoded, *not* null-terminated)
    ///
    const char* data() const noexcept { return str_->data(); }

    /// @brief `true` if the length of the string is `0`
    ///
    bool isEmpty() const noexcept { return str_->isEmpty(); }

    /// @brief The length of the string
    ///
    /// @return number of bytes in the string (*not* characters)
    size_t size() const noexcept { return str_->length(); }

    bool startsWith(const std::string_view s) const noexcept
    {
        return std::string_view(*this).starts_with(s);
    }

    bool endsWith(const std::string_view s) const noexcept
    {
        return std::string_view(*this).ends_with(s);
    }

    bool operator==(const std::string_view sv) const noexcept
    {
        return *str_ == sv;
    }
    bool operator!=(const std::string_view sv) const noexcept
    {
        return *str_ != sv;
    }

    bool operator<(const StringValue& other) const noexcept
    {
        return *str_ < *other.str_;
    }

    operator bool() const noexcept { return !isEmpty(); }   // NOLINT implicit conversion

    /// @fn operator std::string_view() const noexcept
    /// @brief The string as a `std::string_view`
    ///
    /// **Warning:** This `string_view` becomes invalid once the
    /// FeatureStore containing its data is closed.
    ///
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::string_view() const noexcept { return str_->toStringView(); }

    /// @brief Creates this string as a new `std::string`.
    ///
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::string() const { return str_->toString(); }       // may throw

    explicit operator const clarisma::ShortVarString*() const { return str_; }

    /// Pointer to the raw data representing this string.
    ///
    /// **Warning:** This pointer becomes invalid once the
    /// FeatureStore containing its data is closed.
    ///
    const uint8_t* rawBytes() const noexcept { return str_->rawBytes(); }

    template<typename Stream>
    void format(Stream& out) const
    {
        std::string_view sv = str_->toStringView();
        out.write(sv.data(), sv.size());
    }

private:
    const clarisma::ShortVarString* str_;
};

} // namespace geodesk

template<>
struct std::hash<geodesk::StringValue>
{
    size_t operator()(const geodesk::StringValue& s) const noexcept
    {
        std::string_view sv = s;
        return clarisma::Strings::hash(sv.data(), sv.size());
    }
};
