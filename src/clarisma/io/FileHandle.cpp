// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#if defined(_WIN32)
#include "FileHandle_win.cxx"
#elif defined(__linux__) || defined(__APPLE__) 
#include "FileHandle_posix.cxx"
#else
#error "Platform not supported"
#endif

namespace clarisma {

} // namespace clarisma
