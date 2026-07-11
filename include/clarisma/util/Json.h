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

	/// @brief Skips over a JSON object.
	///
	/// Scanning starts immediately after the object's opening brace. Nested objects
	/// are handled, and braces occurring inside JSON strings are ignored. Escaped
	/// characters inside strings are skipped without validating the escape sequence.
	///
	/// This function performs structural scanning only. It does not fully validate
	/// the JSON object.
	///
	/// @param p Pointer to the first character after the opening brace.
	/// @param end Pointer one past the end of the available input.
	/// @return Pointer to the character immediately after the matching closing brace,
	///         or @p end if no matching closing brace is found.
	///
	static const char* skipObject(const char*p, const char* end);

	static const char* skipArray(const char*p, const char* end);

	static const char* skipValue(const char*p, const char* end);

	/// @brief Reads an unescaped string
	///
	/// @param p Pointer to the first character after the opening quote.
	/// @param end Pointer one past the end of the available input.
	///
	/// @return an unescaped string (without enclosing quotes)
	///
	static std::string_view readRawString(const char* pp, const char* end);
};

} // namespace clarisma
