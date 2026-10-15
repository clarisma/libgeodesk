// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <filesystem>
#include <string_view>

namespace clarisma {

namespace AppDirectories
{
	enum class Kind { CACHE, CONFIG, DATA, STATE };

	[[nodiscard]] inline std::filesystem::path tryPath(
		std::string_view vendor,
		std::string_view app,
		std::string_view appId,
		Kind kind);
}

} // namespace clarisma

#if defined(_WIN32)
#include "detail/AppDirectories_win.inl"
#elif defined(__APPLE__)
#include "detail/AppDirectories_macos.inl"
#elif defined(__linux__)
#include "detail/AppDirectories_linux.inl"
#else
#error Unsupported platform
#endif
