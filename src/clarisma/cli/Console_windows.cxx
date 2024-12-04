// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/Console.h>
// #define NOMINMAX
#include <windows.h>
#include <conio.h>  // For getch on Windows
#include <corecrt_io.h>
#include <fcntl.h>

namespace clarisma {

void Console::init()
{
	// hConsole_ = GetStdHandle(STD_ERROR_HANDLE);
	hConsole_ = GetStdHandle(STD_OUTPUT_HANDLE);

	// setvbuf(stdout, nullptr, _IONBF, 0); // 64 * 1024);	// TODO
	// setvbuf(stdout, nullptr, _IOFBF, 1024 * 1024);	// TODO
	// _setmode(_fileno(stdout), _O_BINARY);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	
	if (GetConsoleScreenBufferInfo(hConsole_, &csbi))
	{
		consoleWidth_ = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	}

	DWORD consoleMode;
	GetConsoleMode(hConsole_, &consoleMode);
	prevConsoleMode_ = consoleMode;
	consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	// consoleMode &= ~ENABLE_PROCESSED_INPUT;
	SetConsoleMode(hConsole_, consoleMode);
	if (!SetConsoleOutputCP(CP_UTF8))
	{
		printf("Failed to enable UTF-8 support.\n");  // TODO
	}
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole_, &cursorInfo);
	cursorInfo.bVisible = false; // Set the cursor visibility
	SetConsoleCursorInfo(hConsole_, &cursorInfo);
}


void Console::restore()
{
	// Restore the old console mode
	SetConsoleMode(hConsole_, prevConsoleMode_);
	// Re-enable the cursor
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole_, &cursorInfo);
	cursorInfo.bVisible = true; 
	SetConsoleCursorInfo(hConsole_, &cursorInfo);
}

void Console::print(const char* s, size_t len)
{
	DWORD written;
	// WriteConsoleA(hConsole_, s, static_cast<DWORD>(len), &written, NULL);
	WriteFile(hConsole_, s, static_cast<DWORD>(len), &written, NULL);
}

char Console::readKeyPress()
{
	return _getch();  // Read a single key on Windows
}

} // namespace clarisma