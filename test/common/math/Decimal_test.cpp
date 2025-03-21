// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include "clarisma/math/Decimal.h"

using namespace clarisma;

TEST_CASE("Decimal")
{
	Decimal d("3.5 t");
	REQUIRE(std::isnan(static_cast<double>(d)));
}
