// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <string_view>

namespace clarisma {

class Buffer;

class Csv
{
public:
	/**
	 * @brief Writes a CSV-escaped version of the input string to the buffer.
	 *
	 * If the string contains a comma, double quote, carriage return, or newline,
	 * it is enclosed in double quotes. Embedded double quotes are escaped by doubling
	 * them (i.e., `"` becomes `""`). If no escaping is required, the string is written
	 * as-is, without surrounding quotes.
	 *
	 * Example:
	 * @code
	 * Buffer buf;
	 * Csv::writeEscaped(buf, "hello, \"world\"");
	 * // buf now contains: "hello, ""world"""
	 * @endcode
	 *
	 * @param out The output buffer to write the escaped string to.
	 * @param s   The input string to escape.
	 */
	static void writeEscaped(Buffer& out, std::string_view s);
};

} // namespace clarisma
