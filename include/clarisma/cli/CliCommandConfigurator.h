// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <string>
#include <string_view>

namespace clarisma {

class CliCommand;

class CliCommandConfigurator 
{
public:
    explicit CliCommandConfigurator(CliCommand& command) : command_(command) {}
    bool configure(char* argv[]);
    std::string_view failedArg() const { return failedArg_; }
    std::string_view failureReason() const { return failureReason_; }

private:
    void failed(std::string_view arg, const std::string& reason)
    {
        if(!failedArg_.empty()) return;
        failedArg_ = arg;
        failureReason_ = reason;
    }

    static bool isPotentialOptionValue(const char* s);

    CliCommand& command_;
    std::string_view failedArg_;
    std::string failureReason_;
};

} // namespace clarisma