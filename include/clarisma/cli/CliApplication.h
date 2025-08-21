// Copyright (c) 2023 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/cli/Console.h>
#include <clarisma/cli/ConsoleBuffer.h>

namespace clarisma {

class CliApplication
{
public:
	CliApplication();
	~CliApplication();

	void run(int argc, char* argv[]);

	// ConsoleWriter& out() { return out_; }
	static CliApplication* get() { return theApp_; }

	void fail(std::string msg);
	static void shutdown(const char* msg);

protected:
	Console console_;
	// ConsoleWriter out_;		// TODO: not needed

	static CliApplication* theApp_;
};

} // namespace clarisma
