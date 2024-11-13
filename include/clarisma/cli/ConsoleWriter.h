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

	explicit ConsoleWriter(int mode = 0);

	~ConsoleWriter()
	{
		if(length()) flush();
	}

	ConsoleWriter& timestamp();
	void flush();
	void color(int color);
	void normal();
	bool hasColor() const noexcept { return console_->hasColor(); }

	ConsoleWriter& operator<<(const AnsiColor& color)
	{
		if(hasColor()) writeString(color.data());
		return *this;
	}

	int prompt(bool defaultYes);

	enum
	{
		NONE = 0,
		SUCCESS = 1,
		FAILED = 2,
		LOGGED = 3,
		PROMPT = 4
	};

private:
	void success();
	void failed();
	void prompt();

	DynamicStackBuffer<1024> buf_;
	Console* console_;
	int indent_;
	int timestampSeconds_;
};

} // namespace clarisma
