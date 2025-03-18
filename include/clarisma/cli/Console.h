// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <string_view>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <termios.h>
#endif
#include <clarisma/io/File.h>
#include <clarisma/text/Format.h>

namespace clarisma {

class ConsoleWriter;

/*
 * Console colors:
 * highlight
 * highlight2
 * progress bar
 * progress bar bground
 * progress percent
 * timestamp
 * success
 * error
 * warning
 * orange
 */

class AnsiColor
{
public:
	explicit constexpr AnsiColor() : data_("") {}
	explicit constexpr AnsiColor(const char *s) : data_(s) {}
	constexpr AnsiColor(const AnsiColor& other) = default;
	[[nodiscard]] constexpr const char* data() const { return data_; }

private:
	const char* data_;
};


class Console
{
public:
	enum class Verbosity
	{
		SILENT,
		QUIET,
		NORMAL,
		VERBOSE,
		DEBUG
	};

	friend constexpr std::strong_ordering operator<=>(Verbosity lhs, Verbosity rhs) noexcept
	{
		return static_cast<int>(lhs) <=> static_cast<int>(rhs);
	}

	enum class Stream
	{
		STDOUT,
		STDERR
	};

	enum ConsoleState
	{
		OFF,
		NORMAL,
		PROGRESS
	};

	Console();
	~Console()
	{ 
		theConsole_ = nullptr; 
		restore();
	}

	void init();
	void restore();

	static Verbosity verbosity()
	{
		return get()->verbosity_;
	}

	static void setVerbosity(Verbosity verbosity)
	{
		Console* self = get();
		self->verbosity_ = verbosity;
		self->showProgress_ = (verbosity > Verbosity::QUIET) & self->showProgress_;
		if(verbosity == Verbosity::SILENT) self->consoleState_ = OFF;
	}

	static FileHandle handle(Stream stream)
	{
		Console* self = get();
		return self->handle_[static_cast<int>(stream)];
	}

	static void setOutputFile(FileHandle handle)
	{
		Console* self = get();
		// self->restoreStream(0);
		self->handle_[0] = handle;
		self->isTerminal_[0] = false;
		self->hasColor_[0] = false;
	}

	bool isTerminal(Stream stream) const noexcept
	{
		return isTerminal_[static_cast<int>(stream)];
	}

	bool hasColor(Stream stream) const noexcept
	{
		return hasColor_[static_cast<int>(stream)];
	}

	void enableColor(bool b) noexcept
	{
		hasColor_[0] = isTerminal_[0] & b;
		hasColor_[1] = isTerminal_[1] & b;
	}

	std::chrono::time_point<std::chrono::steady_clock> startTime() const noexcept
	{
		return startTime_;
	}

	void setState(ConsoleState state) { consoleState_ = state;}
	void start(const char* task);

	/// @brief Stops the progress display and returns a ConsoleWriter
	/// to write to stderr.
	///
	static ConsoleWriter end();
	void setTask(const char* task);
	void setProgress(int percentage);
	static void log(std::string_view msg);
	//ConsoleWriter success();
	//ConsoleWriter failed();

	template <size_t N>
	static void log(const char(&msg)[N])
	{
		log(std::string_view(msg, N-1));  // Subtract 1 to exclude null terminator
	}

	static void log(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		log(format, args);
		va_end(args);
	}

	static void log(const char* format, va_list args)
	{
		char buf[1024];
		Format::unsafe(buf, format, args);
		log(std::string_view(buf));
	}

	// static ConsoleWriter log();

	static void msg(std::string_view msg)
	{
		if (theConsole_) theConsole_->log(msg);
	}

	template <size_t N>
	static void msg(const char(&msg)[N])
	{
		msg(std::string_view(msg, N - 1));  // Subtract 1 to exclude null terminator
	}


	static void msg(const char* format, ...)
	{
		if (theConsole_)
		{
			va_list args;
			va_start(args, format);
			theConsole_->log(format, args);
			va_end(args);
		}
	}

	static void debug(const char* format, ...);

	static Console* get() { return theConsole_; }

	void print(Stream stream, const char* s, size_t len);

	char readKeyPress();

	static constexpr AnsiColor DEFAULT{"\033[0m"};
	static constexpr AnsiColor BRIGHT_GREEN{"\033[38;5;84m"};
	static constexpr AnsiColor BRIGHT_ORANGE{"\033[38;5;208m"};
	static constexpr AnsiColor FAINT_LAVENDER{"\033[38;5;147m"};
	static constexpr AnsiColor FAINT_LIGHT_BEIGE{"\033[38;5;217m"};
	static constexpr AnsiColor FAINT_LIGHT_BLUE{"\033[38;5;111m"};
	// static constexpr AnsiColor FAINT_LIGHT_BLUE{"\033[38;5;117m"};
	static constexpr AnsiColor GOLDEN_YELLOW{"\033[38;5;221m"};
	static constexpr AnsiColor GREEN{"\033[38;5;34m"};
	static constexpr AnsiColor HIGHLIGHT_YELLOW{"\033[38;5;148m"};
	static constexpr AnsiColor LAVENDER{"\033[38;5;105m"};
	static constexpr AnsiColor NEON_YELLOW{"\033[38;5;226m"};
	static constexpr AnsiColor WHITE{"\033[38;5;15m"};

private:
	static const char* BLOCK_CHARS_UTF8;
	// static const int MAX_LINE_LENGTH = 78;
	static const int MAX_TASK_CHARS = 38;


	char* formatStatus(char* buf, int secs, int percentage, const char* task) const;
	char* formatProgress(char* p, int percentage) const;
	static char* formatTask(char* p, const char* task) ;
	size_t printWithStatus(char* buf, char* p, std::chrono::steady_clock::duration elapsed,
		int percentage = -1, const char* task = nullptr);
	void displayTimer();
	void initStream(int streamNo);
	void restoreStream(int streamNo);

	static Console* theConsole_;
	FileHandle handle_[2];
	#ifdef _WIN32
	DWORD prevConsoleMode_[2];
    #else
    struct termios prevConsoleMode_[2];
	bool stdinInitialized_ = false;
	#endif
	std::atomic<const char*> currentTask_ = "";
	std::chrono::time_point<std::chrono::steady_clock> startTime_;
	std::atomic<ConsoleState> consoleState_;
	std::atomic<int> currentPercentage_ = -1;
	std::thread thread_;
	int consoleWidth_ = 80;
	bool isTerminal_[2];
	bool hasColor_[2];
	bool showProgress_;
	Verbosity verbosity_;

	friend class ConsoleWriter;
};

} // namespace clarisma
