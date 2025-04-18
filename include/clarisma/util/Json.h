// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <string_view>

namespace clarisma {

class Buffer;

class Json
{
public:
	/**
	 * @brief Writes a JSON-escaped version of the input string to the buffer.
	 *
	 * Escapes control characters (U+0000 to U+001F), double quotes, and backslashes,
	 * according to the JSON specification. Characters outside this range are written
	 * unmodified. Surrounding quotes are not added.
	 *
	 * Example:
	 * @code
	 * Buffer buf;
	 * Json::writeEscaped(buf, "hello\n\"world\"");
	 * // buf now contains: hello\\n\\\"world\\\"
	 * @endcode
	 *
	 * @param out The output buffer to write the escaped string to.
	 * @param s   The input string to escape.
	 */
	static void writeEscaped(Buffer& out, std::string_view s);
};

} // namespace clarisma
