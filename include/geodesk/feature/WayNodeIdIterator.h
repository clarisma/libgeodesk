// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/util/varint.h>
#include <geodesk/feature/WayPtr.h>

namespace geodesk {

///
/// \cond lowlevel
///
class WayNodeIdIterator
{
public:
    explicit WayNodeIdIterator(WayPtr way)
    {
        const uint8_t* p = way.bodyptr();
        remaining_ = static_cast<int>(clarisma::readVarint32(p));
        clarisma::skipVarints(p, remaining_ * 2);
        pId_ = p;
        id_ = 0;
    }

    uint64_t next()
    {
        if(remaining_ == 0) return 0;
        remaining_--;
        id_ += clarisma::readSignedVarint64(pId_);
        return id_;
    }

private:
    const uint8_t* pId_;
    uint64_t id_;
    int remaining_;
};

// \endcond
} // namespace geodesk