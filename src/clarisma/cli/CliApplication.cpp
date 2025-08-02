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

LONG CALLBACK crashHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	/*
	if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		std::cerr << "Caught access violation (segfault).\n";
		// Perform cleanup, logging, etc.
		ExitProcess(1);  // Graceful termination
	}
	return EXCEPTION_CONTINUE_SEARCH;  // Let others handle it too
	*/

	// TODO: For windows, may prefer ExitProcess(code)
	CliApplication::shutdown("Abnormal termination.");
	std::abort();
}

/*		// Does not work on Windows
#ifndef NDEBUG

extern "C" void _assert(const char* exp, const char* file, unsigned line)
{
	char buf[1024];
	Format::unsafe(buf,	"Assertion failed: %s         in %s, line %d",
		exp, file, line);
	CliApplication::shutdown(buf);
	std::abort();
}

#endif
*/

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
		Console::end();
		Console::get()->setState(Console::ConsoleState::OFF);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		{
			ConsoleWriter out(Console::Stream::STDERR);
			out.failed();
			out << msg << "\n";
			out.flush(Console::verbosity() != Console::Verbosity::SILENT);
		}
		Console::get()->restore();
	}
}

CliApplication* CliApplication::theApp_ = nullptr;

CliApplication::CliApplication()
{
	theApp_ = this;
	#ifdef _WIN32
	#ifdef NDEBUG	// don't install crash handler for debug
	SetUnhandledExceptionFilter(crashHandler);
		// Don't use AddVectoredExceptionHandler() because it will
		// catch *any* exception
	#endif
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
