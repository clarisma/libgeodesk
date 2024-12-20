// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/CliApplication.h>

namespace clarisma {

#ifdef _WIN32
#include <windows.h>

BOOL WINAPI consoleHandler(DWORD signal)
{
	if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT)
	{
		CliApplication* theApp = CliApplication::get();
		if(theApp)
		{
			// TODO: This could still race, as the Console
			//  may be in the process of being destroyed
			//  Safer: remove the handler once CliApplication
			//  destructor has been called
			CliApplication::get()->fail("Cancelled.\n");
			Console::get()->restore();
		}
	}
	return FALSE;
}

#endif

CliApplication* CliApplication::theApp_ = nullptr;

CliApplication::CliApplication()
{
	theApp_ = this;
	#ifdef _WIN32
	SetConsoleCtrlHandler(consoleHandler, TRUE);
	#endif
}

CliApplication::~CliApplication()
{
	theApp_ = nullptr;
}

void CliApplication::fail(std::string msg)
{
	console_.failed().writeString(msg);
}

} // namespace clarisma
