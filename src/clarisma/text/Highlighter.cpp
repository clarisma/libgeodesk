// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/text/Highlighter.h>

namespace clarisma {

void Highlighter::highlight(StringBuilder& buf, const char* text, 
                            int ofs, int len, int color)
{
	// TODO: check if colors are supported in terminal
	bool colorize = false;
	// TOOD: find start line
	int start = 0;
	int excess = ofs + len - MAX_LINE_LEN;
	int padding = 0;
	int indent = 4;
	buf.writeRepeatedChar(' ', indent);
	if(excess > 0)
	{
		padding = 3;
		start += excess + padding;
		ofs -= excess + padding;
		buf << "...";
	}
	buf.write(text + start, ofs);
	if (colorize)
	{
		buf << "\033[" << color << 'm';
	}
	buf.write(text + ofs, len);
	if (colorize) buf << "\033[0m";
	buf << (text + ofs + len);
	buf.writeByte('\n');
	buf.writeRepeatedChar(' ', indent + padding + ofs);
	if (colorize)
	{
		buf << "\033[" << color << 'm';
	}
	buf.writeRepeatedChar('^', len);
	if (colorize) buf << "\033[0m";
}

} // namespace clarisma
