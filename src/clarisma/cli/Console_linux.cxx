// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/Console.h>
#include <iostream>
#include <unistd.h> // For write and ioctl
#include <sys/ioctl.h> // For ioctl to get terminal size
#include <stdio.h> // For printf
#include <termios.h>

namespace clarisma {

void Console::init()
{
    // Get console width
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) 
    {
        if(w.ws_col != 0) consoleWidth_ = w.ws_col;
    }

    // Setting the terminal to UTF-8 and cursor visibility can be done via
    // terminal commands if necessary. Typically, cursor visibility and UTF-8
    // settings should be managed by the terminal emulator settings.

    // Hide the cursor via ANSI control code
    std::cout << "\033[?25l" << std::flush;

    struct termios newConsoleMode;
    // Get the current terminal settings
    tcgetattr(STDIN_FILENO, &prevConsoleMode_);
    newConsoleMode = prevConsoleMode_;

    // Disable canonical mode (buffered input) and echoing
    newConsoleMode.c_lflag &= ~(ICANON | ECHO);

    // Set the new terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &newConsoleMode);
}

void Console::restore()
{
    // Restore the old terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &prevConsoleMode_);

    // Re-enable the cursor via ANSI control code
    std::cout << "\033[?25h" << std::flush;
}

void Console::print(const char* s, size_t len)
{
    // Directly write to STDOUT_FILENO
    write(STDOUT_FILENO, s, len);
}

char Console::readKeyPress()
{
    // Read a single character
    return getchar();
}

} // namespace clarisma