// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#if defined(_WIN32)
#include "detail/FileError_win.inl"
#else
#include "detail/FileError_posix.inl"
#endif