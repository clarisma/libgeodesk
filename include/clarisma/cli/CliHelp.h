// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/cli/ConsoleWriter.h>

namespace clarisma {

class CliHelp : public ConsoleWriter
{
public:
    void command(const char* cmd, const char* description);
    void subCommand(const char* name, const char* description);
    void beginSection(const char* sectionName);
    void option(const char* option, const char* description);
    void optionValue(const char* value, const char* description);
    void endSection();

private:
    int formatOptionsAndParams(const char *str);

    int commandIndent_ = 12;
    int optionIndent_ = 26;
    int optionValueIndent_ = 38;
};

} // namespace clarisma