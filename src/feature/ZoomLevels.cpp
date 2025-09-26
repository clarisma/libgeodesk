// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/feature/ZoomLevels.h>
#include <clarisma/validate/Validate.h>

namespace geodesk {

void ZoomLevels::check() const
{
    /*          // TODO: re-enable ???
    if (count() > 8)
    {
        throw clarisma::ValueException("Maximum 8 zoom levels");
    }
    */

    if ((levels_ & 1) == 0)
    {
        throw clarisma::ValueException("Must include root zoom level (0)");
    }

    uint32_t v = levels_;
    while (v)
    {
        int skip = clarisma::Bits::countTrailingZerosInNonZero(v);
        if (skip > 2)
        {
            throw clarisma::ValueException("Must not skip more than 2 levels");
        }
        v >>= (skip + 1);
    }
}

} // namespace geodesk