// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/CliCommand.h>
#include <clarisma/cli/CliCommandConfigurator.h>
#include <clarisma/cli/CliApplication.h>

namespace clarisma {

int CliCommand::run(char* argv[])
{
	CliCommandConfigurator config(*this);
	if(!config.configure(argv))
	{
		Console::end().failed()
			<< Console::HIGHLIGHT_YELLOW << config.failedArg()
			<< Console::DEFAULT	<< ": " << config.failureReason() << '\n';
		return 2;		// Exit code 2 means bad arguments
	}
	return 0;
}


std::string_view CliCommand::getCommand(char* argv[])
{
	char** pArg = &argv[1];
	while (*pArg)
	{
		if (**pArg != '-') return *pArg;
		pArg++;
	}
	return {};
}

} // namespace clarisma
