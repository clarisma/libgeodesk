// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <iosfwd>
#include <clarisma/text/Format.h>
#include <clarisma/compile/unreachable.h>
#include <clarisma/math/Math.h>
#include <geodesk/feature/StringValue.h>
#include <geodesk/feature/TagValues.h>

namespace geodesk {

/// @brief The value of a Tag. Converts implicitly to `std::string`,
/// `double`, `int` or `bool`.
///
/// - `double`: `NaN` if the string value does not start with a valid number
///
/// - `int`: `0` if the string value does not start with a valid number
///
/// - `bool`: `true` unless empty string, `"no"`, or a numerical value of `0`
///
/// **Warning**: A TagValue object is a lightweight wrapper
/// around a pointer to a Feature's tag data, contained
/// in a FeatureStore. It becomes invalid once that
/// FeatureStore is closed. To use its value beyond the
/// lifetime of the FeatureStore, convert it to a `std::string`
/// (which allocates a copy of the tag data), or one of the supported
/// scalar types.
///
/// @see Tag, Tags
///
// TODO: Clarify default value of double()
class GEODESK_API TagValue
{
public:
    TagValue() : taggedNumberValue_(1) {}   // empty string
    explicit TagValue(uint64_t taggedNumberValue, StringValue str = StringValue()) :
        taggedNumberValue_(taggedNumberValue), stringValue_(str) {}
    TagValue(StringValue str) : // NOLINT implicit conversion desired
        taggedNumberValue_(3), stringValue_(str) {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::string() const    // may throw
    {
        char buf[32];
        char *end;

        switch (type())
        {
        case 1:     // global string
        case 3:     // local string (fall through)
            return stringValue_;
        case 0:     // narrow number
            end = clarisma::Format::integer(buf,
                TagValues::intFromNarrowNumber(rawNumberValue()));
            return std::string(buf, end-buf);
        case 2:     // wide number
            return TagValues::decimalFromWideNumber(rawNumberValue());
        default:
            UNREACHABLE_CASE
        }
        return {};  // suppress warning
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator double() const noexcept
    {
        double val;

        switch (type())
        {
        case 1:     // global string
        case 3:     // local string (fall through)
        {
            bool valid = clarisma::Math::parseDouble(stringValue_, &val);
            return valid ? val : 0.0;
            // TODO: Return 0 or NaN?
            //  Python returns 0
        }
        case 0:     // narrow number
            return TagValues::intFromNarrowNumber(rawNumberValue());
        case 2:     // wide number
            return static_cast<double>(TagValues::decimalFromWideNumber(rawNumberValue()));
        default:
            UNREACHABLE_CASE
        }
        // Explicit return to silence warning
        return 0;
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator int64_t() const noexcept
    {
        return static_cast<int64_t>(static_cast<double>(*this));
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator int() const noexcept
    {
        return static_cast<int>(static_cast<double>(*this));
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator clarisma::Decimal() const noexcept
    {
        switch (type())
        {
        case 1:     // global string
        case 3:     // local string (fall through)
            return clarisma::Decimal(stringValue_);
            // TODO: wrong, Decimal() does not ignore non-numeric trailing chars
        case 0:     // narrow number
            return clarisma::Decimal(TagValues::intFromNarrowNumber(
                rawNumberValue()), 0);
        case 2:     // wide number
            return TagValues::decimalFromWideNumber(rawNumberValue());
        default:
            UNREACHABLE_CASE
        }
    }

    // NOLINTNEXTLINE implicit conversion
    operator bool() const noexcept
    {
        switch (type())
        {
        case 1:     // global string
        case 3:     // local string (fall through)
            return stringValue_.size() != 0 && stringValue_ != "no";
            // TODO: Use fixed constant in v2
            // TODO: are "000", "0.00000000" true or false?
        case 0:     // narrow number
            return TagValues::intFromNarrowNumber(rawNumberValue()) != 0;
        case 2:     // wide number
            return TagValues::decimalFromWideNumber(rawNumberValue()) != 0;
        default:
            UNREACHABLE_CASE
        }
        return false;
    }

    bool operator!() const noexcept
    {
        return !static_cast<bool>(*this);
    }

    // TODO: can't rely on types and codes being the same
    //  since other TagValue may come from a different GOL
    bool operator==(const TagValue& other) const noexcept;

    bool operator!=(const TagValue& other) const noexcept
    {
        return !(*this == other);
    }

    bool operator==(const std::string_view &sv) const noexcept
    {
        char buf[32];
        char* end;
        switch (type())
        {
        case 1:     // global string
        case 3:     // local string (fall through)
            return stringValue_ == sv;
        case 0:     // narrow number
            end = clarisma::Format::integer(buf,
                TagValues::intFromNarrowNumber(rawNumberValue()));
            return sv == std::string_view(buf, end-buf);
        case 2:     // wide number
            end = TagValues::decimalFromWideNumber(rawNumberValue()).format(buf);
            return sv == std::string_view(buf, end-buf);
        default:
            UNREACHABLE_CASE
        }
        // Explicit return to silence warning
        return false;
    }

    bool operator==(double val) const noexcept
    {
        return static_cast<double>(*this) == val;
    }

    bool operator==(int64_t val) const noexcept
    {
        return static_cast<int64_t>(*this) == val;
    }

    bool operator!=(const std::string_view& val) const noexcept
    {
        return !(*this == val);
    }

    bool operator!=(double val) const noexcept
    {
        return static_cast<double>(*this) != val;
    }

    bool operator!=(int64_t val) const noexcept
    {
        return !(*this == val);
    }

    bool operator<(const TagValue& other) const noexcept
    {
        return static_cast<int>(*this) < static_cast<int>(other);
    }

    bool operator<(double val) const noexcept
    {
        return static_cast<double>(*this) < val;
    }

    bool operator>(double val) const noexcept
    {
        return static_cast<double>(*this) > val;
    }

    bool operator<=(double val) const noexcept
    {
        return static_cast<double>(*this) <= val;
    }

    bool operator>=(double val) const noexcept
    {
        return static_cast<double>(*this) >= val;
    }

    bool operator<(int64_t val) const noexcept
    {
        return static_cast<int64_t>(*this) < val;
    }

    bool operator>(int64_t val) const noexcept
    {
        return static_cast<int64_t>(*this) > val;
    }

    bool operator<=(int64_t val) const noexcept
    {
        return static_cast<int64_t>(*this) <= val;
    }

    bool operator>=(int64_t val) const noexcept
    {
        return static_cast<int64_t>(*this) >= val;
    }

    bool operator<(int val) const noexcept
    {
        return static_cast<int>(*this) < val;
    }

    bool operator>(int val) const noexcept
    {
        return static_cast<int>(*this) > val;
    }

    bool operator<=(int val) const noexcept
    {
        return static_cast<int>(*this) <= val;
    }

    bool operator>=(int val) const noexcept
    {
        return static_cast<int>(*this) >= val;
    }

    /// @brief Counts the number of UTF-8 characters (*not* bytes)
    /// that this TagValue represents. Numeric values are
    /// assumed to be formatted in their canonical representation.
    ///
    int charCount() const noexcept;

    /// `true` if the value's native storage format is numeric.
    ///
    bool isStoredNumeric() const noexcept
    {
        return (static_cast<int>(taggedNumberValue_) & 1) == 0;
    }

    /// @brief The tag value as a string, if its native storage format
    /// is string. If the value is stored as a number, returns an
    /// empty string.
    ///
    StringValue storedString() const noexcept { return stringValue_; }

    /// @brief The tag value as a Decimal, if its native storage format
    /// is numeric. If the value is stored as a string, returns its
    /// global-string code (or 0 for a local string).
    ///
    clarisma::Decimal storedNumber() const noexcept
    {
        if (type() == TagValues::Type::WIDE_NUMBER)
        {
            return TagValues::decimalFromWideNumber(rawNumberValue());
        }
        return TagValues::decimalFromNarrowNumber(rawNumberValue());
    }

    /// @brief Writes the tag value to the given Buffer,
    /// XMl/HTML-encoded.
    ///
    /// @tparam out  a Buffer
    ///
    // void writeXml(clarisma::Buffer& out);
        // TODO

    template<typename Stream>
    void format(Stream& out) const
    {
        switch (type())
        {
        case 1:     // global string
        case 3:     // local string (fall through)
            stringValue_.format(out);
            break;
        case 0:     // narrow number
        {
            // TODO: make more efficient by encoding in reverse
            char buf[32];
            char* end = clarisma::Format::integer(buf,
                TagValues::intFromNarrowNumber(rawNumberValue()));
            out.write(buf, end - buf);
            break;
        }
        case 2:     // wide number
            TagValues::decimalFromWideNumber(
                rawNumberValue()).format(out);
            break;
        default:
            UNREACHABLE_CASE
        }
    }

private:
    int type() const { return static_cast<int>(taggedNumberValue_) & 3; }
    uint_fast32_t rawNumberValue() const
    {
        return static_cast<uint_fast32_t>(taggedNumberValue_ >> 2);
    }

    uint64_t taggedNumberValue_;
    StringValue stringValue_;
};

inline std::ostream& operator<<(std::ostream& out, const TagValue& v)
{
    v.format(out);
    return out;
}

/*
inline clarisma::Buffer& operator<<(clarisma::Buffer& out, const TagValue& v)
{
    v.format(out);
    return out;
}
*/

} // namespace geodesk
