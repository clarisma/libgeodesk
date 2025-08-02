// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <iostream>
#include "clarisma/math/Decimal.h"

using namespace clarisma;


TEST_CASE("Decimal")
{
	char buf[64];

	Decimal d(".5");
	REQUIRE(d == 0.5);
	d.format(buf);
	REQUIRE(std::string(buf) == "0.5");

	d = Decimal("");
	REQUIRE(!d.isValid());
	d = Decimal("", true);
	REQUIRE(!d.isValid());

	d = Decimal("0");
	REQUIRE(d == 0);
	d = Decimal("0", true);
	REQUIRE(d == 0);
	d.format(buf);
	REQUIRE(std::string(buf) == "0");

	d= Decimal("3.5 t");
	REQUIRE(d == 3.5);

	d = Decimal("3.5 t", true);
	REQUIRE(!d.isValid());

	d = Decimal("50", false);
	REQUIRE(d == 50);

	d = Decimal("50", true);
	REQUIRE(d == 50);

	d = Decimal("01", true);
	REQUIRE(!d.isValid());

	d = Decimal("01", false);
	REQUIRE(d == 1);

	d = Decimal("0.0", true);
	REQUIRE(d == 0);

	d = Decimal("0.00", true);
	REQUIRE(d == 0);

	d = Decimal("0.500", false);
	REQUIRE(d == .5);

	d = Decimal("0.500", true);
	REQUIRE(d == .5);

	d = Decimal("00.500", true);
	REQUIRE(!d.isValid());

	d = Decimal("0.", false);
	REQUIRE(d == 0);

	d = Decimal("0.", true);
	REQUIRE(!d.isValid());

	d = Decimal("", false);
	REQUIRE(!d.isValid());

	d = Decimal(".25", false);
	REQUIRE(d == .25);

	d = Decimal(".25", true);
	REQUIRE(!d.isValid());

	d = Decimal("-0.0000", false);
	REQUIRE(d == 0);

	d = Decimal("-0.0000", true);
	REQUIRE(!d.isValid());

	d = Decimal("4.25.", false);
	REQUIRE(d == 4.25);

	d = Decimal("4.25.", true);
	REQUIRE(!d.isValid());

	d = Decimal("1000000000000000000000000000", false);
	REQUIRE(!d.isValid());

	d = Decimal("1000000000000000000000000000", true);
	REQUIRE(!d.isValid());
}

TEST_CASE("Decimal bug #22")
{
	Decimal d = Decimal("0.061", false);
	REQUIRE(d.isValid());
	REQUIRE(d == 0.061);
	char buf[32];
	d.format(buf);
	REQUIRE(std::string(buf) == "0.061");

	d = Decimal("-0.0099", false);
	REQUIRE(d.isValid());
	REQUIRE(d == -0.0099);
	d.format(buf);
	REQUIRE(std::string(buf) == "-0.0099");
}