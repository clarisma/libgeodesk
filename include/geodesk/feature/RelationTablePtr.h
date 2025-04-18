// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/DataPtr.h>

namespace geodesk {

/// \cond lowlevel
///
class RelationTablePtr
{
public:
	RelationTablePtr(DataPtr p) : p_(p) {}
	DataPtr ptr() const { return p_; }

private:
	DataPtr p_;
};

// \endcond
} // namespace geodesk
