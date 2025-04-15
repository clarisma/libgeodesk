// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/filter/Filters.h>
#include <geodesk/feature/FeatureUtils.h>
#include <geodesk/feature/QueryException.h>
#include <geodesk/feature/View.h>
#include <geodesk/filter/PredicateFilter.h>

namespace geodesk {

/// \cond

template<typename P>
class FeatureBase;
template<typename T>
class FeatureIterator;
class Filter;
class MatcherHolder;
class PreparedFilterFactory;

// forward-declare the derived classes, so we can declare them as friends
// (otherwise, the derived classes can only access the base class of the 
// same specialization)

class Features;
class Nodes;
class Ways;
class Relations;

// TODO: type restrictions on derived

template <typename T>
class FeaturesBase
{
public:
    FeaturesBase(const FeaturesBase<T>& other) :
        view_(other.view_)
    {
    }

    static FeaturesBase empty(FeatureStore* store) noexcept
    {
        return FeaturesBase(View(store));
    }

    FeaturesBase& operator=(const FeaturesBase& other)
    {
        view_ = other.view_;
        return *this;
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator bool() const { return !FeatureUtils::isEmpty(view_); }
    bool operator !() const {return FeatureUtils::isEmpty(view_); }

    template<typename T2>
    [[nodiscard]] FeaturesBase operator&(const FeaturesBase<T2>& other) const;

    
    /// @name Query by type & tags
    /// @{

    [[nodiscard]] FeaturesBase operator()(const char* query) const
    {
        return FeaturesBase(view_.withQuery(query));
    }

    [[nodiscard]] FeaturesBase operator()(const Feature& feature) const
    {
        return intersecting(feature);
    }

    /// @brief Returns all features whose bounding box intersects
    /// the given bounding box.
    ///
    /// @param box
    [[nodiscard]] FeaturesBase operator()(const Box& box) const
    {
        return { view_.withBounds(box) };
    }

    /// @brief Returns all features whose bounding box contains
    /// the given Coordinate.
    ///
    /// @param xy
    [[nodiscard]] FeaturesBase operator()(Coordinate xy) const
    {
        return { view_.withBounds(xy) };
    }
    
    /// @}
    /// @name Single-feature queries
    /// @{

    [[nodiscard]] std::optional<T> first() const;
    [[nodiscard]] T one() const;

    // NOLINTNEXTLINE(google-explicit-constructor)
    [[nodiscard]] operator std::vector<T>() const;

    void addTo(std::vector<T>& v) const;

    /// @}
    /// @name Scalar queries
    /// @{

    /// Returns the number of features in this collection.
    ///
    [[nodiscard]] uint64_t count() const
    {
        return FeatureUtils::count(view_);
    }

    /// @brief Calculates the total length (in meters) of the features
    /// in this collection.
    ///
    [[nodiscard]] double length() const;

    /// @brief Calculates the total area (in square meters) of the features
    /// in this collection.
    ///
    [[nodiscard]] double area() const;

    FeatureIterator<T> begin() const;

    std::nullptr_t end() const
    {
        return nullptr;  // Simple sentinel indicating the end of iteration
    }

    /// @}
    /// @name Spatial filters
    /// @{

    /// @brief Returns all features whose geometry intersects with the
    /// given Feature.
    ///
    [[nodiscard]] FeaturesBase intersecting(const Feature& feature) const
    {
        return { view_.withFilter(Filters::intersects(feature))};
    }

    /// @brief Returns all features that lie entirely inside the geometry
    /// of the given Feature.
    ///
    [[nodiscard]] FeaturesBase within(const Feature& feature) const
    {
        return {view_.withFilter(Filters::within(feature))};
    }

    /// @brief Only features whose geometry contains the
    /// given Coordinate.
    ///
    [[nodiscard]] FeaturesBase containing(Coordinate xy) const
    {
        return {view_.withFilter(Filters::containsPoint(xy))};
    }

    /// @brief Only features whose geometry contains the
    /// given location.
    ///
    /// @param lon degrees longitude
    /// @param lat degrees latitude
    ///
    [[nodiscard]] FeaturesBase containingLonLat(double lon, double lat) const
    {
        return {view_.withFilter(Filters::containsPoint(
            Coordinate::ofLonLat(lon, lat)))};
    }

    /// @brief Only features whose geometry crosses the
    /// given Feature.
    ///
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    [[nodiscard]] FeaturesBase crossing(const Feature& feature) const
    {
        return {view_.withFilter(Filters::crossing(feature))};
    }

    /// @brief Only features whose closest point lies within
    /// `distance` meters of `xy`.
    ///
    /// @param distance the maximum distance (in meters)
    /// @param xy the center of the search radius
    ///
    [[nodiscard]] FeaturesBase maxMetersFrom(double distance, Coordinate xy) const
    {
        return {view_.withFilter(Filters::maxMetersFrom(distance, xy))};
    }

    /// @brief Only features whose closest point lies within
    /// `distance` meters of the given location.
    ///
    /// @param distance the maximum distance (in meters)
    /// @param lon degrees longitude of the center of the search radius
    /// @param lat degrees latitude of the center of the search radius
    ///
    [[nodiscard]] FeaturesBase maxMetersFromLonLat(double distance, double lon, double lat) const
    {
        return {view_.withFilter(Filters::maxMetersFrom(distance,
            Coordinate::ofLonLat(lon, lat)))};
    }

    /// @}
    /// @name Topological filters
    /// @{

    template<typename P>
    [[nodiscard]] FeaturesBase membersOf(FeatureBase<P> feature) const
    {
        if(feature.isRelation()) return {view_.membersOf(RelationPtr(feature.ptr()))};
        if(feature.isWay()) return {view_.nodesOf(WayPtr(feature.ptr()))};
        return {empty()};
    }

    template<typename P>
    [[nodiscard]] FeaturesBase nodesOf(FeatureBase<P> feature) const
    {
        if(feature.isWay()) return {view_.nodesOf(WayPtr(feature.ptr()))};
        return {empty()};
    }

    template<typename P>
    [[nodiscard]] FeaturesBase parentsOf(FeatureBase<P> feature) const
    {
        if (feature.isAnonymousNode())
        {
            return {view_.parentWaysOf(feature.xy())};
        }
        return {view_.parentsOf(feature.ptr())};
    }

    /// @}

    template <typename Predicate>
    [[nodiscard]] FeaturesBase filter(Predicate predicate) const
    {
        return FeaturesBase(view_.withFilter(
            new PredicateFilter<Predicate>(predicate)));
    }

    /// @brief Obtains a Key for the given string, which can be
    /// used for fast tag-value lookups.
    ///
    /// **Important:** The resulting Key can only be used for
    /// features that are stored in the same GOL.
    ///
    [[nodiscard]] Key key(std::string_view k) const
    {
        return store()->key(k);
    }

    /// @brief Returns a pointer to the FeatureStore
    /// which contains the features in this collection.
    ///
    FeatureStore* store() const noexcept { return view_.store(); }

protected:
    FeaturesBase(const View& view) : view_(view)
    {
    }

    View empty() const
    {
        return view_.empty();
    }

    FeatureIterator<T> query() const;

    bool isEmpty() const
    {
        return FeatureUtils::isEmpty(view_);
    }


    static View rootView(const char* golFile)
    {
        FeatureStore* store = FeatureStore::openSingle(golFile);
        const MatcherHolder* matcher = store->getAllMatcher();
        return View(View::WORLD, 0, FeatureTypes::ALL, store,
            Box::ofWorld(), matcher, nullptr);
    }

    FeaturesBase withFilter(PreparedFilterFactory& factory, Feature feature) const;

    View view_;

    friend class FeatureBase<FeaturePtr>;
    friend class FeatureBase<NodePtr>;
    friend class FeatureBase<WayPtr>;
    friend class FeatureBase<RelationPtr>;
    friend class Features;
    friend class Nodes;
    friend class Ways;
    friend class Relations;
};

// \endcond

} // namespace geodesk

