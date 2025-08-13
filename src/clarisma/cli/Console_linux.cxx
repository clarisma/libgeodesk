// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/Console.h>
#include <iostream>
#include <unistd.h> // For write and ioctl
#include <sys/ioctl.h> // For ioctl to get terminal size
#include <stdio.h> // For printf
#include <termios.h>

namespace clarisma {

void Console::initStream(int streamNo)
{
    if(!stdinInitialized_)
    {
        struct termios newConsoleMode;
        // Get the current terminal settings
        tcgetattr(STDIN_FILENO, &prevConsoleMode_[0]);
        newConsoleMode = prevConsoleMode_[0];
            // always use stream 0

        // Disable canonical mode (buffered input) and echoing
        newConsoleMode.c_lflag &= ~(ICANON | ECHO);

        // Set the new terminal settings
        tcsetattr(STDIN_FILENO, TCSANOW, &newConsoleMode);
        stdinInitialized_ = true;
    }

    FileHandle handle = (streamNo==0) ? STDOUT_FILENO : STDERR_FILENO;
    handle_[streamNo] = handle;
    isTerminal_[streamNo] = isatty(handle.native());
    if(!isTerminal_[streamNo])
    {
        hasColor_[streamNo] = false;
        return; // No further init
    }
    hasColor_[streamNo] = true;

    // Get console width
    struct winsize w;
    if (ioctl(handle.native(), TIOCGWINSZ, &w) == 0)
    {
        if(w.ws_col != 0) consoleWidth_ = w.ws_col;
    }

    // Setting the terminal to UTF-8 and cursor visibility can be done via
    // terminal commands if necessary. Typically, cursor visibility and UTF-8
    // settings should be managed by the terminal emulator settings.

    // Hide the cursor via ANSI control code
    // std::cout << "\033[?25l" << std::flush;
    write(handle.native(), "\033[?25l", 6);
}

void Console::restoreStream(int streamNo)
{
    if(stdinInitialized_)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &prevConsoleMode_[0]);
            // always use stream 0
        stdinInitialized_ = false;
    }

    if(!isTerminal_[streamNo]) return;

    // Re-enable the cursor via ANSI control code
    // std::cout << "\033[?25h" << std::flush;
    write(handle_[streamNo].native(), "\033[?25h", 6);
}

void Console::print(Stream stream, const char* s, size_t len)
{
    int streamNo = static_cast<int>(stream);
    // Directly write to STDOUT_FILENO
    write(handle_[streamNo].native(), s, len);
}

char Console::readKeyPress()
{
    // Read a single character
    return getchar();
}

} // namespace clarisma