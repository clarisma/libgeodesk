// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <string_view>


class UrlView 
{
public:
    UrlView(std::string_view url);

private:
    std::string_view host_;
    std::string_view path_;
    std::string_view query_;
    std::string_view fragment_;
    int port_;
};
