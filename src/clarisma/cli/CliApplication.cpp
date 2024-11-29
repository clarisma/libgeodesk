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
		CliApplication* theApp = CliApplication::get();
		if(theApp)
		{
			// TODO: This could still race, as the Console
			//  may be in the process of being destroyed
			//  Safer: remove the handler once CliApplication
			//  destructor has been called
			Console::get()->end();
			//printf("\n\nEnded progress\n\n");
			Console::get()->setState(Console::ConsoleState::OFF);
			//printf("\n\nTurned console off\n\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			//printf("\n\nStarted final msg\n\n");
			{
				ConsoleWriter out(ConsoleWriter::CANCELLED);
				out << "Cancelled.\n";
			}
			//printf("\n\nFlushed final msg\n\n");
			Console::get()->restore();
			//printf("\n\nConsole restored\n\n");

			// Unregister the handler to avoid further signals during cleanup
			SetConsoleCtrlHandler(consoleHandler, FALSE);
		}
	}
	return FALSE;
}

#else

void signalHandler(int signal)
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
    std::exit(128 + signal);
}

#endif

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
