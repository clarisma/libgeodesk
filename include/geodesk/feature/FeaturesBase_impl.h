// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FeaturesBase.h>
#include <geodesk/feature/FeatureIterator.h>

// \cond

namespace geodesk {

template<typename T>
FeatureIterator<T> FeaturesBase<T>::query() const
{
    return FeatureIterator<T>(*this);
}

template<typename T>
FeatureIterator<T> FeaturesBase<T>::begin() const
{
    return FeatureIterator<T>(*this);
}


template<typename T>
[[nodiscard]] std::optional<T> FeaturesBase<T>::first() const
{
    FeatureIterator<T> query(*this);
    if(query != nullptr) return std::optional<T>(*query);
    return std::nullopt;
}

template<typename T>
[[nodiscard]] T FeaturesBase<T>::one() const
{
    FeatureIterator<T> query(*this);
    if(query != nullptr)
    {
        T feature = *query;
        ++query;
        if(query != nullptr)
        {
            throw QueryException("More than one feature found");
        }
        return feature;
    }
    throw QueryException("No feature found");
}

template<typename T>
void FeaturesBase<T>::addTo(std::vector<T>& v) const
{
    for(T f: *this) v.push_back(f);
}

template<typename T>
[[nodiscard]] double FeaturesBase<T>::area() const
{
    double total = 0;
    for(auto f: *this) total += f.area();
    return total;
}

template<typename T>
[[nodiscard]] double FeaturesBase<T>::length() const
{
    double total = 0;
    for(auto f: *this) total += f.length();
    return total;
}

template<typename T>
[[nodiscard]] FeaturesBase<T>::operator std::vector<T>() const
{
    std::vector<T> v;
    addTo(v);
    return v;
}

} // namespace geodesk

// \endcond
