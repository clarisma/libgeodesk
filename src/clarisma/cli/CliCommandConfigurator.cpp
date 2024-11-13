// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/cli/CliCommandConfigurator.h>
#include <clarisma/cli/CliCommand.h>
#include <cassert>

namespace clarisma {

bool CliCommandConfigurator::isPotentialOptionValue(const char* s)
{
    if(s)
    {
        if(s[0] == '-') return std::isdigit(s[1]);
        return true;
    }
    return false;
}

bool CliCommandConfigurator::configure(char* argv[])
{
    int paramCount = 0;
    int n = 0;
    for (;;)
    {
        n++;
        char* pArg = argv[n];
        if (!pArg) break;
        if (*pArg == '-')
        {
            // Option
            char* pName = pArg + ((pArg[1] == '-') ? 2 : 1);
            char* pValue = pName;
            size_t nameLen;
            for(;;)
            {
                if (*pValue == 0)
                {
                    nameLen = pValue - pName;
                    break;
                }
                if (*pValue == '=')
                {
                    nameLen = pValue - pName;
                    *pValue++ = 0;
                    break;
                }
                pValue++;
            }
            std::string_view name(pName, nameLen);
            std::string_view value(pValue);
            int accepted;
            if(value.empty())
            {
                // For "-o", we try to use the following argument (if any) as value
                if(isPotentialOptionValue(argv[n+1]))
                {
                    // Try "-o <value>"
                    try
                    {
                        accepted = command_.setOption(name, argv[n+1]);
                        if(accepted >= 0)
                        {
                            n += accepted;
                            // If arg accepted as value (1), we'll skip the arg
                            continue;
                        }
                    }
                    catch (const std::exception& ex)
                    {
                        // If <value> is invalid, try if the option is accepted
                        // without value
                        try
                        {
                            accepted = command_.setOption(name, std::string_view(""));
                            if(accepted == 0) continue;		// option accepted without value
                            assert(accepted > 0);
                        }
                        catch(const std::exception& ex2)
                        {
                            // do nothing, fall through and report the
                            // original exception
                        }
                        failed(name, ex.what());
                        n++;
                        continue;
                    }
                    failed(name, "Invalid option");
                    continue;
                }
            }
            try
            {
                accepted = command_.setOption(name, value);
                if(accepted == 1) continue;
                if(accepted == 0)
                {
                    // Return value 0 means the option does not allow a value
                    if(!value.empty())
                    {
                        // Report error if a value was supplied anyway
                        failed(name, "Option does not take a value");
                    }
                    continue;
                }
            }
            catch (const std::exception& ex)
            {
                failed(name, ex.what());
                continue;
            }
            failed(name, "Invalid option");
        }
        else
        {
            // Positional parameter
            if (!command_.setParam(paramCount++, pArg))
            {
                failed(pArg, "Invalid parameter");
            }
        }
    }
    return failedArg_.empty();
}

} // namespace clarisma



