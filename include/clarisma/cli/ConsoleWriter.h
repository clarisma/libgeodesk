// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/cli/Console.h>
#include <clarisma/util/StreamWriter.h>
#include <clarisma/util/DynamicStackBuffer.h>

namespace clarisma {

class ConsoleWriter : public AbstractStreamWriter<ConsoleWriter>
{
public:
	using AbstractStreamWriter::operator<<;

	explicit ConsoleWriter(Console::Stream stream = Console::Stream::STDOUT);
	ConsoleWriter(const ConsoleWriter&) = delete;

	~ConsoleWriter()
	{
		if(length()) flush();
	}

	ConsoleWriter& timestamp();
	ConsoleWriter& success();
	ConsoleWriter& failed();
	ConsoleWriter& arrow();

	void flush(bool forceDisplay = false);
	void color(int color);
	void normal();
	bool hasColor() const noexcept { return hasColor_; }

	ConsoleWriter& operator<<(const AnsiColor& color)
	{
		if(hasColor()) writeString(color.data());
		return *this;
	}

	int prompt(bool defaultYes);

private:
	DynamicStackBuffer<1024> buf_;
	Console* console_;
	// uint16_t indent_;
	uint8_t stream_;
	bool isTerminal_;
	bool hasColor_;
	int timestampSeconds_;
};

} // namespace clarisma
