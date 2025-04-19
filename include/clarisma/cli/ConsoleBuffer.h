// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/DynamicStackBuffer.h>
#include <clarisma/cli/Console.h>

namespace clarisma {

class ConsoleBuffer : public DynamicStackBuffer<1024>
{
public:
	explicit ConsoleBuffer(Console::Stream stream = Console::Stream::STDOUT);
	ConsoleBuffer(const ConsoleBuffer&) = delete;

   ~ConsoleBuffer()
	{
		// printf("\n\n%p\nLength = %lld\n\n", this, length());
		if(length()) flush();
		// printf("\n\nFlushed CW\n\n");
	}

	ConsoleBuffer& blank();
	ConsoleBuffer& timestamp();
	ConsoleBuffer& success();
	ConsoleBuffer& failed();
	ConsoleBuffer& arrow();

	void flush(bool forceDisplay = false);
	void color(int color);
	void normal();
	bool hasColor() const noexcept { return hasColor_; }

	ConsoleBuffer& operator<<(const AnsiColor& color)
	{
		if(hasColor()) write(color.data());
		return *this;
	}

	int prompt(bool defaultYes);

	void filled(char* p) override;
	void flush(char* p) override;

private:
  	void ensureNewlineUnsafe();

	Console* console_;
	// uint16_t indent_;
	uint8_t stream_;
	bool isTerminal_;
	bool hasColor_;
	int timestampSeconds_;
};

} // namespace clarisma
