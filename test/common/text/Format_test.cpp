// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <iostream>
#include "clarisma/math/Decimal.h"

using namespace clarisma;
using namespace clarisma::Format;


TEST_CASE("Format::formatDouble")
{
	char buf[64];

	formatDouble(buf, 12.345, 2, false);
	REQUIRE(std::string(buf) == "12.35");
}