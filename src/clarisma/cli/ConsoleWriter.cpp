// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/ConsoleWriter.h>
#include <clarisma/cli/Console.h>

namespace clarisma {

ConsoleWriter::ConsoleWriter() :
	console_(Console::get()),
	indent_(0),
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
	if(console_->consoleState_ == Console::ConsoleState::PROGRESS)
	{
		ensureCapacityUnsafe(256);
		if(timestampSeconds_ < 0)
		{
			auto elapsed = std::chrono::steady_clock::now() - console_->startTime();
			timestampSeconds_ = static_cast<int>(
				std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
		}
		p_ = console_->formatStatus(p_, timestampSeconds_,
			console_->currentPercentage_, console_->currentTask_);
	}
	else
	{
		if(console_->consoleState_ == Console::ConsoleState::OFF) [[unlikely]]
		{
			if(!forceDisplay) return;
		}
		writeConstString("\033[K");	// clear remainder of line
			// TODO: needed? Why not clear start of line?
	}
	console_->print(data(), length());
	clear();
}

ConsoleWriter& ConsoleWriter::timestamp()
{
	ensureCapacityUnsafe(64);
	putStringUnsafe("\033[2K");	// clear current line
	auto elapsed = std::chrono::steady_clock::now() - console_->startTime();
	int ms = static_cast<int>(
		std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
	div_t d;
	d = div(ms, 1000);
	int s = d.quot;
	ms = d.rem;

	bool color = console_->hasColor();
	if(color) writeConstString("\033[38;5;242m");
	p_ = Format::timer(p_, s, ms);
	if(color)
	{
		writeConstString("\033[0m  ");
	}
	else
	{
		writeConstString("  ");
	}
	timestampSeconds_ = s;
	indent_ = 14;
	return *this;
}

ConsoleWriter& ConsoleWriter::success()
{
	bool color = console_->hasColor();
	ensureCapacityUnsafe(64);
	putStringUnsafe("\033[2K");	// clear current line
	if(color) putStringUnsafe("\033[97;48;5;28m");
		// Or use 64 for apple green
	auto elapsed = std::chrono::steady_clock::now() - console_->startTime();
	int secs = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
	p_ = Format::timer(p_, secs, -1);
	if(color)
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
	putStringUnsafe("\r\033[2K");	// clear current line
	if(console_->hasColor())
	{
		putStringUnsafe("\033[38;5;9m ─────── \033[0m");
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
	putStringUnsafe("\033[2K");	// clear current line
	if(console_->hasColor())
	{
		putStringUnsafe("\033[38;5;148m ──────▶ \033[0m");
	}
	else
	{
		putStringUnsafe(" ------> ");
	}
	return *this;
}

int ConsoleWriter::prompt(bool defaultYes)
{
	ensureCapacityUnsafe(64);
	if(console_->hasColor())
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
	flush();
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
	console_->print("\r\033[2K", 5);
	return res;
}

} // namespace clarisma
