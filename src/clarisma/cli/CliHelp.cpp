// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/CliHelp.h>

namespace clarisma {

void CliHelp::command(const char* cmd, const char* description)
{
    *this << "Usage: " << Console::WHITE;
    formatOptionsAndParams(cmd);
    *this << "\n\n" << description << "\n\n";
}


void CliHelp::indent(int chars)
{

}

void CliHelp::subCommand(const char* name, const char* description)
{
    *this << "  " << Console::GOLDEN_YELLOW << name << Console::DEFAULT;
    writeRepeatedChar(' ', commandIndent_ - strlen(name) - 2);
    *this << description << '\n';
}

void CliHelp::beginSection(const char* sectionName)
{
    *this << Console::WHITE << sectionName << Console::DEFAULT << "\n";
}

void CliHelp::endSection()
{
    *this << "\n";
}

int CliHelp::formatOptionsAndParams(const char *str)
{
    int mode = -1; // 0=normal, 1=option, 2=param, -1 = default
    const char* p = str;
    while(*p != 0)
    {
        char ch = *p++;
        if(ch == '-')
        {
            if(mode <= 0)
            {
                mode = 1;
                *this << Console::GOLDEN_YELLOW;
            }
        }
        else if(ch == '<')
        {
            mode = 2;
            *this << Console::FAINT_LIGHT_BLUE;
        }
        else if (ch == '>')
        {
            writeByte(ch);
            *this << Console::DEFAULT;
            mode = 0;
            continue;
        }
        else if(strchr(",=|[]/", ch))
        {
            if(mode != 0)
            {
                mode = 0;
                *this << Console::DEFAULT;
            }
        }
        writeByte(ch);
    }
    *this << Console::DEFAULT;
    return static_cast<int>(p - str);
}

// -s, --long=[<param>]
void CliHelp::option(const char* option, const char* description)
{
    *this << "  ";
    int optionWidth = formatOptionsAndParams(option) + 2;
    if(optionWidth > optionIndent_ - 2)
    {
        *this << '\n';
        optionWidth = 0;
    }
    writeRepeatedChar(' ', optionIndent_ - optionWidth);
    *this << description << '\n';
}


void CliHelp::optionValue(const char* value, const char* description)
{
    writeRepeatedChar(' ', optionIndent_);
    *this << Console::FAINT_LIGHT_BLUE << value << Console::DEFAULT;
    writeRepeatedChar(' ', optionValueIndent_ - optionIndent_ - strlen(value));
    *this << description << '\n';
}

} // namespace clarisma