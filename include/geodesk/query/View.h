// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/query/ViewBase.h>

namespace geodesk {

/// \cond lowlevel

class GEODESK_API View : public ViewBase<View>
{
public:
    static View create(int view, int flags, FeatureTypes types, const FeatureStore* store,
        Context context, const MatcherHolder* matcher, const Filter* filter)
    {
        return { view, flags, types, store, context, matcher, filter };
    }

    // TODO: Don't use the current matcher; use the allMatcher of the store
    //  (otherwise complex matchers hang around longer than needed)
    View getEmpty() const
    {
        store()->addref();
        matcher()->addref();
        return View(EMPTY, 0, 0, store(), FeaturePtr(), matcher(), nullptr);
    }

private:
    using ViewBase::ViewBase;
};

/// \endcond lowlevel

} // namespace geodesk
