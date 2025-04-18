// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdlib>
#include <cstring>
#include <string_view>
#include <clarisma/util/Unicode.h>

namespace clarisma {

class BufferWriter;

class Xml
{
public:
  	/// @brief Replaces XML entities in-place.
  	/// Invalid entities are silently removed.
    ///
    /// @return pointer to the terminating null char
  	static char* unescapeInplace(char* s);

	// TODO: change to Buffer
	static void writeEscaped(BufferWriter& out, std::string_view s);
};

} // namespace clarisma
