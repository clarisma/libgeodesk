// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/text/Format.h>
#include <clarisma/text/Parsing.h>
#include <clarisma/util/streamable.h> // for << operator support

namespace clarisma {

/// @brief Semantic version X.Y.Z packed into a 64-bit integer.
///
/// Layout (MSB -> LSB):
/// - bits 63..48: major (0..65535)
/// - bits 47..32: minor (0..65535)
/// - bits 31..16: patch (0..65535)
/// - bits 15..0 : reserved (0xffff for now)
///
/// If parsing fails, data_ is set to 0 (which is also 0.0.0).
///
class SemanticVersion
{
public:
    /// @brief Default construct to 0.0.0.
    constexpr SemanticVersion() noexcept :
        data_(0)
    {
    }

    /// @brief Construct from components.
    /// @param major Major version (0..65535)
    /// @param minor Minor version (0..65535)
    /// @param patch Patch version (0..65535)
    ///
    /// If any component is negative or exceeds 65535, the version
    /// is treated as invalid and becomes 0.0.0 (data_ == 0).
    SemanticVersion(int major, int minor, int patch) noexcept
    {
        if (major < 0 || minor < 0 || patch < 0
            || major > 0xFFFF || minor > 0xFFFF || patch > 0xFFFF)
        {
            data_ = 0;
            return;
        }

        data_ = pack(static_cast<std::uint16_t>(major),
            static_cast<std::uint16_t>(minor),
            static_cast<std::uint16_t>(patch));
    }

    /// @brief Construct from a string "MAJOR.MINOR.PATCH".
    ///
    /// Pre-release and build metadata are ignored; the string is
    /// truncated at the first '-' or '+' if present.
    ///
    /// Valid examples:
    /// - "1.2.3"
    /// - "1.2.3-alpha"     (parsed as 1.2.3)
    /// - "1.2.3+build.5"   (parsed as 1.2.3)
    ///
    /// If the string cannot be parsed as exactly three numeric
    /// components separated by '.', data_ is set to 0.
    explicit SemanticVersion(std::string_view sv) noexcept
    {
        parseFromString(sv);
    }

    /// @brief Major component.
    int major() const noexcept
    {
        return static_cast<int>((data_ >> 48) & 0xFFFFu);
    }

    /// @brief Minor component.
    int minor() const noexcept
    {
        return static_cast<int>((data_ >> 32) & 0xFFFFu);
    }

    /// @brief Patch component.
    int patch() const noexcept
    {
        return static_cast<int>((data_ >> 16) & 0xFFFFu);
    }

    /// @brief Returns true if the encoded value is non-zero.
    ///
    /// Note: 0.0.0 is indistinguishable from an unparsable value.
    explicit operator bool() const noexcept
    {
        return data_ != 0;
    }

    /// @brief Three-way comparison, using packed integer ordering.
    constexpr auto operator<=>(const SemanticVersion &other) const noexcept
        = default;

    /// @brief Equality comparison.
    constexpr bool operator==(const SemanticVersion &other) const noexcept
        = default;

    char* formatReverse(char* end) const
    {
        end = Format::unsignedIntegerReverse(static_cast<unsigned int>(patch()), end) - 1;
        *end = '.';
        end = Format::unsignedIntegerReverse(static_cast<unsigned int>(minor()), end) - 1;
        *end = '.';
        return Format::unsignedIntegerReverse(static_cast<unsigned int>(major()), end);
    }

    template<typename S>
    void format(S& out) const
    {
        char buf[64];
        char* end = &buf[sizeof(buf)];
        char* start = formatReverse(end);
        out.write(start, end - start);
    }

private:
    /// @brief Pack version components into the internal layout.
    static constexpr std::uint64_t pack(std::uint16_t major,
        std::uint16_t minor,
        std::uint16_t patch) noexcept
    {
        return (static_cast<std::uint64_t>(major) << 48)
            | (static_cast<std::uint64_t>(minor) << 32)
            | (static_cast<std::uint64_t>(patch) << 16);
    }

    /// @brief Parse from string, setting data_.
    void parseFromString(std::string_view sv) noexcept
    {
        // Trim off pre-release and build metadata.
        const std::size_t cut = sv.find_first_of("-+");
        if (cut != std::string_view::npos)
        {
            sv = sv.substr(0, cut);
        }

        // Find dots for MAJOR.MINOR.PATCH.
        const std::size_t dot1 = sv.find('.');
        const std::size_t dot2 =
            (dot1 == std::string_view::npos)
                ? std::string_view::npos
                : sv.find('.', dot1 + 1);

        // Must have exactly two dots.
        if (dot1 == std::string_view::npos
            || dot2 == std::string_view::npos)
        {
            data_ = 0;
            return;
        }

        // No third dot allowed.
        if (sv.find('.', dot2 + 1) != std::string_view::npos)
        {
            data_ = 0;
            return;
        }

        std::string_view majStr = sv.substr(0, dot1);
        std::string_view minStr = sv.substr(dot1 + 1, dot2 - dot1 - 1);
        std::string_view patStr = sv.substr(dot2 + 1);

        if (majStr.empty() || minStr.empty() || patStr.empty())
        {
            data_ = 0;
            return;
        }

        try
        {
            // TODO: these number parsing functions don't report
            //   errors, they simply stop at first non-numeric
            const std::uint64_t maj64 = Parsing::parseUnsignedLong(majStr);
            const std::uint64_t min64 = Parsing::parseUnsignedLong(minStr);
            const std::uint64_t pat64 = Parsing::parseUnsignedLong(patStr);

            if (maj64 > 0xFFFFu || min64 > 0xFFFFu || pat64 > 0xFFFFu)
            {
                data_ = 0;
                return;
            }

            data_ = pack(static_cast<std::uint16_t>(maj64),
                static_cast<std::uint16_t>(min64),
                static_cast<std::uint16_t>(pat64));
        }
        catch (...)
        {
            data_ = 0;
        }
    }

    std::uint64_t data_;
};

} // namespace clarisma
