// Copyright (c) 2025 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only


namespace clarisma {

inline bool File::tryRename(const char* from, const char* to)
{
    return MoveFileEx(from, to, MOVEFILE_REPLACE_EXISTING);
}

inline void File::rename(const char* from, const char* to)
{
    if (!tryRename(from, to))
    {
        throw IOException();
    }
}


} // namespace clarisma
