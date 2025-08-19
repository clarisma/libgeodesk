// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

namespace clarisma {

/// \class TextTemplate
///
/// \brief Minimal template engine with arena layout.
/// @details
/// Arena layout (all offsets relative to &total_area_len_):
///   u32  text_ofs
///   Zero or more:
///     u32 literal_len
///     u32 param_len
///   Zero or more:
///     (characters of text and params)
///
/// Whitespace inside `{ ... }` is allowed and trimmed.
///
/// Example template:
///   "Hello {fname} {lname}"
/// Segments written as: literal "Hello ", param "fname",
/// literal " ", param "lname"
///
class TextTemplate
{
public:
    /// \brief Compile a template string into a Template.
    ///
    /// @param text Source template (UTF-8).
    /// @return Owning unique pointer.
    ///
    static std::unique_ptr<TextTemplate> compile(std::string_view text);

    /// \brief Render into a buffer using a lookup callback
    ///
    /// @tparam Buf Buffer with write(const char*, size_t).
    /// @tparam Lookup Callable: std::string_view(std::string_view name).
    ///
    /// @param buf Output sink
    /// @param lookup Parameter resolver
    ///
    template <typename Buf, typename Lookup>
    void write(Buf& buf, Lookup&& lookup) const
    {
        const uint32_t* p = reinterpret_cast<const uint32_t*>(arena_);
        const char* pTextStart = arena_ + *p;
        const char* pText = pTextStart;

        ++p;
        for (;;)
        {
            uint32_t len = *p++;
            buf.write(pText, len);
            pText += len;
            if (reinterpret_cast<const char*>(p) == pTextStart) break;
            len = *p++;
            std::string_view name(pText, len);
            pText += len;
            std::string_view value = lookup(name);
            buf.write(value.data(), value.size());
        }
    }

private:
    TextTemplate() noexcept = default;

    /*
    /// \brief Private constructor; use compile().
    Template() noexcept
    {
        *reinterpret_cast<uint32_t*>(arena_) = 4;
        *reinterpret_cast<uint32_t*>(arena_ + 4) = 0;
    }
    */

    alignas(uint32_t) char arena_[2 * sizeof(uint32_t)];
};


} // namespace clarisma
