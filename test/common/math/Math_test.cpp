// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include "clarisma/math/Math.h"

using namespace clarisma;

TEST_CASE("Math::parseDouble")
{
  	double d;
	bool success = Math::parseDouble("3.5 t", &d);
	REQUIRE(success);
	REQUIRE(d == 3.5);

	success = Math::parseDouble("-0001000100", &d);
	REQUIRE(success);
	REQUIRE(d == -1000100);

	success = Math::parseDouble("4.99999.555", &d);
	REQUIRE(success);
	REQUIRE(d == 4.99999);

	success = Math::parseDouble("12345678.9123000", &d);
	REQUIRE(success);
	REQUIRE(d == 12345678.9123);

	success = Math::parseDouble("1977-09-24", &d);
	REQUIRE(success);
	REQUIRE(d == 1977);

	success = Math::parseDouble("", &d);
	REQUIRE(!success);
	REQUIRE(std::isnan(d));

	success = Math::parseDouble("not_a_number", &d);
	REQUIRE(!success);
	REQUIRE(std::isnan(d));

	success = Math::parseDouble("-monkey", &d);
	REQUIRE(!success);
	REQUIRE(std::isnan(d));

	success = Math::parseDouble("..1", &d);
	REQUIRE(!success);
	REQUIRE(std::isnan(d));

	success = Math::parseDouble("--20", &d);
	REQUIRE(!success);
	REQUIRE(std::isnan(d));
}
