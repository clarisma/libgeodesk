// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <cassert>
#include <cstring>
#include <clarisma/cli/ConsoleWriter.h>
#include <clarisma/thread/Threads.h>

#if defined(_WIN32)
#include "Console_windows.cxx"
#elif defined(__linux__) || defined(__APPLE__) 
#include "Console_linux.cxx"
#else
#error "Platform not supported"
#endif


namespace clarisma {

// Always print status after logging a line
// always print if task changed
// If time is different, update timestamp
// If % changed, update % and scrollbar

Console* Console::theConsole_ = nullptr;

template <size_t N>
static char* putString(char* p, const char(&s)[N])
{
	memcpy(p, s, N-1);	// Subtract 1 to exclude null terminator
	return p + N-1;
}

Console::Console() :
	consoleState_(ConsoleState::NORMAL)
{
	init();
	theConsole_ = this;
}

void Console::start(const char* task)
{
	startTime_ = std::chrono::steady_clock::now();
	currentPercentage_.store(0, std::memory_order_release);
	char buf[256];
	char* p = formatStatus(buf, 0, 0, task);
	assert(p-buf < sizeof(buf));
	print(buf, p-buf);
	consoleState_.store(ConsoleState::PROGRESS, std::memory_order_release);
	thread_ = std::thread(&Console::displayTimer, this);
}

// TODO: This suffers from a (benign) race condition where the output
// of different threads could appear out of order (log output with an
// earlier timestamp follows output with a later timestamp).
// We could obtain and format the current time *last* to mitigate 
// its impact (especially if the time to format text varies greatly)
// However, we then would no longer have the true timestamp of when the 
// logged event occurred
// Full synchronization via a mutex would fix this, but the performance
// impact may be unacceptable (we would also have to sync the updates
// of the progress bar)

void Console::log(std::string_view msg)
{
	ConsoleWriter out(ConsoleWriter::LOGGED);
	out.writeString(msg);
	out.writeByte('\n');
	/*
	auto elapsed = std::chrono::steady_clock::now() - startTime_;
	char buf[1024];
	int ms = static_cast<int>(
		std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
	div_t d;
	d = div(ms, 1000);
	int s = d.quot;
	ms = d.rem;

	char* p = putString(buf, "\u001b[38;5;242m");
	p = Format::timer(p, s, ms);
	p = putString(p, "\u001b[0m");
	size_t maxTextLen = consoleWidth_ - 15;
	memset(p, ' ', maxTextLen + 2);
	p += 2;
	size_t actualTextLen = std::min(msg.length(), maxTextLen);
	memcpy(p, msg.data(), actualTextLen);
	p += maxTextLen;
	*p++ = '\n';
	int percentage = currentPercentage_.load(std::memory_order_acquire);
	const char* task = currentTask_.load(std::memory_order_acquire);
	size_t bytesPrinted = printWithStatus(buf, p, elapsed, percentage, task);
	assert(bytesPrinted < sizeof(buf));
	*/
}

/*
ConsoleWriter Console::log()
{
	ConsoleWriter out(ConsoleWriter::LOGGED);
	return out;
}
*/

const char* Console::BLOCK_CHARS_UTF8 = (const char*)
	u8"\U00002588"	// full block
	u8"\U0000258E"	// one-quarter block
	u8"\U0000258C"	// half block
	u8"\U0000258A"	// three-quarter block
	;

/// Generates the characters to display the progress
/// percentage and progress bar.
/// It does NOT null-terminate the buffer.
///
/// @param p the location of the first percentage digit
/// @param percentage
/// @return pointer to the character after the end of the
///		progress bar (which is either a space or a thin
///		left vertical bar)
char* Console::formatProgress(char* p, int percentage) const
{
	// Progress percentage

	// if(hasColor_) p = putString(p, "\033[33m");
	if(hasColor_) p = putString(p, "\033[38;5;172m");	// 70
	div_t d;
	int v1, v2, v3;
	d = div(percentage, 10);
	v3 = d.rem;
	d = div(d.quot, 10);
	v2 = d.rem;
	v1 = d.quot;
	*p++ = v1 ? ('0' + v1) : ' ';
	*p++ = (v1 | v2) ? ('0' + v2) : ' ';
	*p++ = '0' + v3;
	*p++ = '%';

	// Progress bar

	*p++ = ' ';
	// if(hasColor_) p = putString(p, "\033[33;100m");
	if(hasColor_) p = putString(p, "\033[38;5;172;48;5;236m");
		// 28 is slightly more saturated
	int fullBlocks = percentage / 4;
	char* pEnd = p + fullBlocks * 3;
	while (p < pEnd)
	{
		*p++ = BLOCK_CHARS_UTF8[0];
		*p++ = BLOCK_CHARS_UTF8[1];
		*p++ = BLOCK_CHARS_UTF8[2];
	}
	int partialBlocks = percentage % 4;
	int emptyBlocks;
	if (partialBlocks)
	{
		const char* pChar = &BLOCK_CHARS_UTF8[partialBlocks * 3];
		*pEnd++ = *pChar++;
		*pEnd++ = *pChar++;
		*pEnd++ = *pChar;
		emptyBlocks = 25 - fullBlocks - 1;
	}
	else
	{
		emptyBlocks = 25 - fullBlocks;
	}
	p = pEnd;
	pEnd = p + emptyBlocks;
	while (p < pEnd)
	{
		*p++ = ' ';
	}
	if(hasColor_)
	{
		p = putString(p, "\033[0m ");
	}
	else
	{
		// If we're not displaying colors (and hence users won't see
		// the background of the progress bar), use a thin line
		// to mark the end of the progress bar
		p = putString(p, "\xE2\x96\x8F");
	}
	return p;
}

/// Writes the task name to the given buffer, and also adds
/// an ANSI instruction to clear the rest of the line, as well
/// as a carriage return. It does NOT null-terminate the buffer.
///
/// @return pointer to the character beyond the CR
///
char* Console::formatTask(char* p, const char* task)
{
	char* pEnd = p + MAX_TASK_CHARS;
	while (*task && p < pEnd)
	{
		*p++ = *task++;
	}
	p = putString(p, "\033[K\r");	// clear rest of line and CR
	return p;
}

char* Console::formatStatus(char* buf, int secs, int percentage, const char* task) const
{
	char* p = Format::timer(buf, secs, -1);
	*p++= ' ';
	p = formatProgress(p, percentage);
	p = formatTask(p, task);
	return p;
}


void Console::setProgress(int percentage)
{
	int oldPercentage = currentPercentage_.load(std::memory_order_relaxed);
	if (percentage != oldPercentage)
	{
		currentPercentage_.store(percentage, std::memory_order_release);
		char buf[256];
		char* p = putString(buf, "\033[9C");
		p = formatProgress(p, percentage);
		*p++ = '\r';
		assert(p-buf < sizeof(buf));
		print(buf, p-buf);
	}
}


size_t Console::printWithStatus(char* buf, char* p,
	std::chrono::steady_clock::duration elapsed,
	int percentage, const char* task)
{
	int secs = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
	const char* end = formatStatus(p, secs, percentage, task);
	size_t size = end - buf;
	print(buf, size);
	return size;
}


void Console::setTask(const char* task)
{
	currentTask_.store(task, std::memory_order_release);
	char buf[256];
	char* p = putString(buf, "\033[40C");	// skip 40 chars
	p = formatTask(p, task);
	assert(p-buf < sizeof(buf));
	print(buf, p-buf);
}


void Console::debug(const char* format, ...)
{
	if (theConsole_)
	{
		char buf[1024];
		char* p = buf;
		*p++ = '[';
		std::string threadId = Threads::currentThreadId();
		memcpy(p, threadId.data(), threadId.length());
		p += threadId.length();
		*p++ = ']';
		*p++ = ' ';
		va_list args;
		va_start(args, format);
		Format::unsafe(p, format, args);
		va_end(args);
		theConsole_->log(std::string_view(buf));
	}
}


/*
ConsoleWriter Console::finish(TaskResult result)
{
	ConsoleWriter out;

	consoleState_.store(ConsoleState::NORMAL, std::memory_order_release);
	if(thread_.joinable()) thread_.detach();

	char buf[1024];
	char* p = buf;

	switch(result)
	{
	case TaskResult::NONE:
		// TODO
		break;
	case TaskResult::SUCCESS:
	{
		if(hasColor_) p = putString(buf, "\033[97;48;5;64m");
		auto elapsed = std::chrono::steady_clock::now() - startTime_;
		int secs = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
		p = Format::timer(p, secs, -1);
		break;
	}
	case TaskResult::FAILED:
		if(hasColor_) p = putString(buf, "\033[38;5;15m\033[48;5;160m");
		p = putString(buf, " FAILED ");
		break;
	case TaskResult::CANCELLED:
		if(hasColor_) p = putString(buf, "\033[38;5;9m");
		p = putString(buf, " ────── ");
		break;
	}
	if(hasColor_) p = putString(p, "\033[0m");
	p = putString(p, "  ");
	size_t maxMsgChars = consoleWidth_ - 10;
	size_t msgLen = std::min(maxMsgChars, msg.size());
	memcpy(p, msg.data(), msgLen);
	p = putString(p + msgLen, "\033[K\n");	// clear rest of line and newline
	assert(p-buf < sizeof(buf));
	print(buf, p-buf);
}
*/

void Console::end()
{
	consoleState_.store(ConsoleState::NORMAL, std::memory_order_release);
	if(thread_.joinable()) thread_.detach();
}

ConsoleWriter Console::success()
{
	end();
	return ConsoleWriter(ConsoleWriter::SUCCESS);
}

ConsoleWriter Console::failed()
{
	end();
	return ConsoleWriter(ConsoleWriter::FAILED);
}

void Console::displayTimer()
{
	while (consoleState_.load() >= ConsoleState::PROGRESS)
	{
		char buf[16];
		auto now = std::chrono::steady_clock::now();
		auto elapsed = now - startTime_;
		int secs = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
		char* p = Format::timer(buf, secs, -1);
		*p++ = '\r';
		print(buf, p-buf);
		auto nextUpdate = time_point_cast<std::chrono::seconds>(now) + std::chrono::seconds(1);
		std::this_thread::sleep_until(nextUpdate);
	}
}

} // namespace clarisma
