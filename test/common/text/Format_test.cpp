// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <iostream>
#include <string>
#include "clarisma/text/Format.h"

using namespace clarisma;
using namespace clarisma::Format;


TEST_CASE("Format::formatDouble")
{
	char buf[64];

	formatDouble(buf, 12.345, 2, false);
	REQUIRE(std::string(buf) == "12.35");

	formatDouble(buf, -12, 5, true);
	REQUIRE(std::string(buf) == "-12.00000");

	formatDouble(buf, -12.1, 5, true);
	REQUIRE(std::string(buf) == "-12.10000");

	formatDouble(buf, -9999, 5, false);
	REQUIRE(std::string(buf) == "-9999");

	formatDouble(buf, 0, 7, false);
	REQUIRE(std::string(buf) == "0");

	formatDouble(buf, -18.9999, 0, false);
	REQUIRE(std::string(buf) == "-19");

	formatDouble(buf, -0.30000, 0, true);
	REQUIRE(std::string(buf) == "0");

	formatDouble(buf, 0.5);
	REQUIRE(std::string(buf) == "0.5");

	formatDouble(buf, 0.5, 0, true);
	REQUIRE(std::string(buf) == "1");
}