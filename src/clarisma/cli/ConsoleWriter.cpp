// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/ConsoleWriter.h>
#include <thread>
#include <clarisma/cli/Console.h>

namespace clarisma {

ConsoleWriter::ConsoleWriter(Console::Stream stream) :
	console_(Console::get()),
	// indent_(0),
	stream_(static_cast<uint8_t>(stream)),
	isTerminal_(console_->isTerminal(stream)),
	hasColor_(console_->hasColor(stream)),
	timestampSeconds_(-1)
{
	setBuffer(&buf_);
}

void ConsoleWriter::color(int color)
{
	writeConstString("\033[38;5;");
	formatInt(color);
	writeByte('m');
}

void ConsoleWriter::normal()
{
	writeConstString("\033[0m");
}


void ConsoleWriter::flush(bool forceDisplay)
{
	if(console_->isTerminal_[stream_])
	{
		if(console_->consoleState_ == Console::ConsoleState::PROGRESS)
		{
			ensureCapacityUnsafe(256);
			if(peekLastChar() != '\n') *p_++ = '\n';
			if(timestampSeconds_ < 0)
			{
				auto elapsed = std::chrono::steady_clock::now() - console_->startTime();
				timestampSeconds_ = static_cast<int>(
					std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
			}
			p_ = console_->formatStatus(p_, timestampSeconds_,
				console_->currentPercentage_, console_->currentTask_);
			timestampSeconds_ = -1;
		}
		else
		{
			if(console_->consoleState_ == Console::ConsoleState::OFF) [[unlikely]]
			{
				if(!forceDisplay) return;
			}
			if(peekLastChar() != '\n') writeByte('\n');
		}
	}
	else
	{
		if(peekLastChar() != '\n') writeByte('\n');
	}
	console_->print(static_cast<Console::Stream>(stream_), data(), length());
	clear();
}

// TODO: Don't confuse wih clear(), which empties the buffer of
//  underlying the BufferWriter
ConsoleWriter& ConsoleWriter::blank()
{
	if(isTerminal_)
	{
		ensureCapacityUnsafe(8);
		putStringUnsafe("\033[2K");	// clear current line
	}
	return *this;
}

ConsoleWriter& ConsoleWriter::timestamp()
{
	ensureCapacityUnsafe(64);
	if(isTerminal_) putStringUnsafe("\033[2K");	// clear current line
	auto elapsed = std::chrono::steady_clock::now() - console_->startTime();
	int ms = static_cast<int>(
		std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
	div_t d;
	d = div(ms, 1000);
	int s = d.quot;
	ms = d.rem;

	if(hasColor()) writeConstString("\033[38;5;242m");
	p_ = Format::timer(p_, s, ms);
	if(hasColor())
	{
		writeConstString("\033[0m  ");
	}
	else
	{
		writeConstString("  ");
	}
	timestampSeconds_ = s;
	// indent_ = 14;
	return *this;
}

ConsoleWriter& ConsoleWriter::timestampAndThread()
{
	std::ostringstream oss;
	oss << std::this_thread::get_id();
	timestamp() << " [" << oss.str() << "] ";
	return *this;
}

ConsoleWriter& ConsoleWriter::success()
{
	ensureCapacityUnsafe(64);
	if(isTerminal_) putStringUnsafe("\033[2K");	// clear current line
	if(hasColor()) putStringUnsafe("\033[97;48;5;28m");
		// Or use 64 for apple green
	auto elapsed = std::chrono::steady_clock::now() - console_->startTime();
	int secs = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
	p_ = Format::timer(p_, secs, -1);
	if(hasColor())
	{
		putStringUnsafe("\033[0m ");
	}
	else
	{
		putStringUnsafe(" ");
	}
	return *this;
}

ConsoleWriter& ConsoleWriter::failed()
{
	ensureCapacityUnsafe(64);
	if(isTerminal_) putStringUnsafe("\r\033[2K");	// clear current line
	if(hasColor())
	{
		// putStringUnsafe("\033[38;5;9m ─────── \033[0m");
		putStringUnsafe("\033[38;5;15;48;5;1m ────── \033[0m ");
			// 160 is brigher red
	}
	else
	{
		putStringUnsafe(" ------- ");
	}
	return *this;
}

ConsoleWriter& ConsoleWriter::arrow()
{
	ensureCapacityUnsafe(64);
	if(isTerminal_) putStringUnsafe("\033[2K");	// clear current line
	if(hasColor())
	{
		putStringUnsafe("\033[38;5;148m ──────▶ \033[0m");
	}
	else
	{
		putStringUnsafe(" ------> ");
	}
	return *this;
}

// TODO: How does this work for silent mode, or if stderr is redirected?
int ConsoleWriter::prompt(bool defaultYes)
{
	ensureCapacityUnsafe(64);
	if(hasColor())
	{
		if(defaultYes)
		{
			putStringUnsafe(" [\033[38;5;148mY\033[0m/n]");
		}
		else
		{
			putStringUnsafe(" [y/\033[38;5;148mN\033[0m]");
		}
	}
	else
	{
		if(defaultYes)
		{
			putStringUnsafe(" [Y/n]");
		}
		else
		{
			putStringUnsafe(" [y/N]");
		}
	}
	console_->print(static_cast<Console::Stream>(stream_), data(), length());
	clear();

	int res = defaultYes;
	for(;;)
	{
		char key = console_->readKeyPress();
		if(key == '\n' || key == '\r') break;
		if(key == 'y' || key == 'Y')
		{
			res = 1;
			break;
		}
		if(key == 'n' || key == 'N')
		{
			res = 0;
			break;
		}
		if(key == 3 || key == 27)
		{
			res = -1;
			break;
		}
	}
	console_->print(Console::Stream::STDERR, "\r\033[2K", 5);
	return res;
}

} // namespace clarisma
