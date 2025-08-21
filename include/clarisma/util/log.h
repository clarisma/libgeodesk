// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <stdio.h>
#include <clarisma/cli/ConsoleBuffer.h>

namespace clarisma {

#ifndef NDEBUG
#define LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...) do {} while (0)
#endif

#define FORCE_LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)

#ifndef NDEBUG
#define LOGS clarisma::ConsoleBuffer(clarisma::Console::Stream::STDERR).timestamp()
#else
#define LOGS if(false) clarisma::ConsoleBuffer().timestamp()
#endif


} // namespace clarisma
