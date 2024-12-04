// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/CliApplication.h>
#include <csignal>

namespace clarisma {

#ifdef _WIN32
#include <windows.h>

BOOL WINAPI consoleHandler(DWORD signal)
{
	if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT)
	{
		CliApplication::shutdown("Cancelled.");
		// Unregister the handler to avoid further signals during cleanup
		SetConsoleCtrlHandler(consoleHandler, FALSE);
	}
	return FALSE;
}

#else

void signalHandler(int signal)
{
    CliApplication::shutdown("Cancelled.");
    std::exit(128 + signal);
}

#endif

void terminateHandler()
{
	CliApplication::shutdown("Abnormal termination.");
	std::abort();
}

void CliApplication::shutdown(const char* msg)
{
	CliApplication* theApp = get();
	if(theApp)
	{
		// TODO: This could still race, as the Console
		//  may be in the process of being destroyed
		//  Safer: remove the handler once CliApplication
		//  destructor has been called
		Console::get()->end();
		Console::get()->setState(Console::ConsoleState::OFF);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		{
			ConsoleWriter out;
			out << msg << "\n";
			out.flush(true);
		}
		Console::get()->restore();
	}
}

CliApplication* CliApplication::theApp_ = nullptr;

CliApplication::CliApplication()
{
	theApp_ = this;
	#ifdef _WIN32
	SetConsoleCtrlHandler(consoleHandler, TRUE);
    #else
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
	#endif
	std::set_terminate(terminateHandler);
}

CliApplication::~CliApplication()
{
	theApp_ = nullptr;
}

void CliApplication::fail(std::string msg)
{
	console_.end().failed().writeString(msg);
}

} // namespace clarisma
