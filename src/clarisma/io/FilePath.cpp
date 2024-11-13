// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <clarisma/io/FilePath.h>

namespace clarisma {

const char* FilePath::extension(const char* filename, size_t len)
{
	const char* p = filename + len - 1;
	while (p > filename && *p != '.' && *p != '/' && *p != '\\')
	{
		p--;
	}
	return *p == '.' ? p : "";
}


std::string_view FilePath::name(std::string_view path)
{
    return path.substr(path.find_last_of("/\\:") + 1);
}



} // namespace clarisma
