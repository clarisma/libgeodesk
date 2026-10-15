// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <filesystem>
#include <catch2/catch_test_macros.hpp>
#include <geodesk/geodesk.h>

using namespace geodesk;

inline Features getMonaco()
{
    const std::filesystem::path golPath =
        std::filesystem::path{TEST_DATA_DIR} / "monaco.gol";
    std::string golFileName = golPath.string();
    return Features(golFileName.c_str());
}