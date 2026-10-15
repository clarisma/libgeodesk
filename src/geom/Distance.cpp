// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/geom/Distance.h>

namespace geodesk {

double Distance::pointSegmentSquared(double x1, double y1, double x2, double y2,
    double px, double py)
{
    x2 -= x1;
    y2 -= y1;
    px -= x1;
    py -= y1;
    double dotprod = px * x2 + py * y2;
    double projlenSquared;
    if (dotprod <= 0.0)
    {
        projlenSquared = 0.0;
    }
    else
    {
        px = x2 - px;
        py = y2 - py;
        dotprod = px * x2 + py * y2;
        if (dotprod <= 0.0)
        {
            projlenSquared = 0.0;
        }
        else
        {
            projlenSquared = dotprod * dotprod / (x2 * x2 + y2 * y2);
        }
    }
    double lenSquared = px * px + py * py - projlenSquared;
    return (lenSquared < 0) ? 0 : lenSquared;
}

} // namespace geodesk
