// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <stdio.h>
#include <clarisma/cli/ConsoleWriter.h>

namespace clarisma {

#ifndef NDEBUG
#define LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...) do {} while (0)
#endif

#define FORCE_LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)

#ifndef NDEBUG
#define LOGS clarisma::ConsoleWriter(clarisma::Console::Stream::STDERR).timestamp()
#else
#define LOGS if(false) clarisma::ConsoleWriter().timestamp()
#endif


} // namespace clarisma
