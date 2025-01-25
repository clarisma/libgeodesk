// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/Console.h>
// #define NOMINMAX
#include <windows.h>
#include <conio.h>  // For getch on Windows
#ifdef _MSC_VER
#include <corecrt_io.h> // For MSVC's _isatty
#else
#include <unistd.h>     // For GCC/MinGW's isatty
#endif

// Define a cross-platform alias for isatty
#ifdef _MSC_VER
#define isatty _isatty
#endif

#include <fcntl.h>

namespace clarisma {

void Console::initStream(int streamNo)
{
	HANDLE hConsole = GetStdHandle((streamNo==0) ?
		STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
	handle_[streamNo] = hConsole;
	isTerminal_[streamNo] = isatty(_fileno(streamNo == 0 ? stdout : stderr));
	if(!isTerminal_[streamNo])
	{
		hasColor_[streamNo] = false;
		return; // No further init
	}
	hasColor_[streamNo] = true;
	// TODO: use GETENV to check for NOCOLOR

	// setvbuf(stdout, nullptr, _IONBF, 0); // 64 * 1024);	// TODO
	// setvbuf(stdout, nullptr, _IOFBF, 1024 * 1024);	// TODO
	// _setmode(_fileno(stdout), _O_BINARY);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	
	if (GetConsoleScreenBufferInfo(hConsole, &csbi))
	{
		consoleWidth_ = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	}

	DWORD consoleMode;
	GetConsoleMode(hConsole, &consoleMode);
	prevConsoleMode_[streamNo] = consoleMode;
	consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	// consoleMode &= ~ENABLE_PROCESSED_INPUT;
	SetConsoleMode(hConsole, consoleMode);
	if (!SetConsoleOutputCP(CP_UTF8))
	{
		printf("Failed to enable UTF-8 support.\n");  // TODO
	}

	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = false; // Set the cursor visibility
	SetConsoleCursorInfo(hConsole, &cursorInfo);
}


void Console::restoreStream(int streamNo)
{
	if(!isTerminal_[streamNo]) return;

	// Restore the old console mode
	SetConsoleMode(handle_[streamNo], prevConsoleMode_[streamNo]);
	// Re-enable the cursor
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(handle_[streamNo], &cursorInfo);
	cursorInfo.bVisible = true; 
	SetConsoleCursorInfo(handle_[streamNo], &cursorInfo);
}

void Console::print(Stream stream, const char* s, size_t len)
{
	int streamNo = static_cast<int>(stream);
	DWORD written;
	// WriteConsoleA(hConsole_, s, static_cast<DWORD>(len), &written, NULL);
	WriteFile(handle_[streamNo], s, static_cast<DWORD>(len), &written, NULL);
}

char Console::readKeyPress()
{
	return _getch();  // Read a single key on Windows
}

} // namespace clarisma