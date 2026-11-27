// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <array>
#include <optional>
#include <geodesk/filter/Filters.h>
#include <geodesk/feature/FeatureUtils.h>
#include <geodesk/feature/QueryException.h>
#include <geodesk/query/View.h>
#include <geodesk/filter/PredicateFilter.h>
#ifdef GEODESK_WITH_GEOS
#include <geos_c.h>
#endif

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
class FeaturesBase : public View
{
public:
    FeaturesBase(const FeaturesBase<T>& other) :
        View(other)
    {
        addref();
    }

    ~FeaturesBase()
    {
        release();
    }

    /*
    static FeaturesBase empty(FeatureStore* store) noexcept
    {
        return FeaturesBase(View(store));
    }
    */

    FeaturesBase& operator=(const FeaturesBase& other)
    {
        other.addref();
        release();
        // TODO: set from other
        return *this;
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator bool() const { return !isEmpty(); }
    bool operator !() const {return isEmpty(); }

    template<typename T2>
    [[nodiscard]] FeaturesBase operator&(const FeaturesBase<T2>& other) const
    {
        return withOther(other);
    }

    
    /// @name Query by type & tags
    /// @{

    [[nodiscard]] FeaturesBase operator()(const char* query) const
    {
        return withQuery(query);
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
        return withBounds(box);
    }

    /// @brief Returns all features whose bounding box contains
    /// the given Coordinate.
    ///
    /// @param xy
    [[nodiscard]] FeaturesBase operator()(Coordinate xy) const
    {
        return withBounds(xy);
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
        return withFilter(Filters::intersects(feature));
    }

    /// @brief Returns all features that lie entirely inside the geometry
    /// of the given Feature.
    ///
    [[nodiscard]] FeaturesBase within(const Feature& feature) const
    {
        return withFilter(Filters::within(feature));
    }

    /// @brief Only features whose geometry contains the
    /// given Coordinate.
    ///
    [[nodiscard]] FeaturesBase containing(Coordinate xy) const
    {
        return withFilter(Filters::containsPoint(xy));
    }

    /// @brief Only features whose geometry contains the
    /// given location.
    ///
    /// @param lon degrees longitude
    /// @param lat degrees latitude
    ///
    [[nodiscard]] FeaturesBase containingLonLat(double lon, double lat) const
    {
        return withFilter(Filters::containsPoint(
            Coordinate::ofLonLat(lon, lat)));
    }

    /// @brief Only features whose geometry crosses the
    /// given Feature.
    ///
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    [[nodiscard]] FeaturesBase crossing(const Feature& feature) const
    {
        return withFilter(Filters::crossing(feature));
    }

    /// @brief Only features whose closest point lies within
    /// `distance` meters of `xy`.
    ///
    /// @param distance the maximum distance (in meters)
    /// @param xy the center of the search radius
    ///
    [[nodiscard]] FeaturesBase maxMetersFrom(double distance, Coordinate xy) const
    {
        return withFilter(Filters::maxMetersFrom(distance, xy));
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
        return withFilter(Filters::maxMetersFrom(distance,
            Coordinate::ofLonLat(lon, lat)));
    }

#ifdef GEODESK_WITH_GEOS
    /// @brief Only features whose geometry intersects @p geom.
    ///
    /// This method is only available if build option `GEODESK_WITH_GEOS`
    /// is enabled (off by default).
    ///
    /// @param context  GEOS context associated with @p geom
    /// @param geom     GEOS geometry used as the intersection filter
    ///
    FeaturesBase intersecting(GEOSContextHandle_t context, const GEOSGeometry* geom) const
    {
        return { view_.withFilter(Filters::intersecting(context, geom))};
    }

    /// @brief Only features that lie entirely inside @p geom.
    ///
    /// This method is only available if build option `GEODESK_WITH_GEOS`
    /// is enabled (off by default).
    ///
    /// @param context  GEOS context associated with @p geom
    /// @param geom     GEOS geometry used as the containment filter
    ///
    FeaturesBase within(GEOSContextHandle_t context, const GEOSGeometry* geom) const
    {
        return { view_.withFilter(Filters::within(context, geom))};
    }

    /// @brief Only features whose geometry contains @p geom.
    ///
    /// This method is only available if build option `GEODESK_WITH_GEOS`
    /// is enabled (off by default).
    ///
    /// @param context  GEOS context associated with @p geom
    /// @param geom     GEOS geometry that must lie entirely within
    ///                 each returned feature's geometry
    ///
    FeaturesBase containing(GEOSContextHandle_t context, const GEOSGeometry* geom) const
    {
        return { view_.withFilter(Filters::containing(context, geom))};
    }

    /// @brief Only features whose geometry crosses @p geom.
    ///
    /// This method is only available if build option `GEODESK_WITH_GEOS`
    /// is enabled (off by default).
    ///
    /// @param context  GEOS context associated with @p geom
    /// @param geom     GEOS geometry used as the crossing filter
    ///
    FeaturesBase crossing(GEOSContextHandle_t context, const GEOSGeometry* geom) const
    {
        return { view_.withFilter(Filters::crossing(context, geom))};
    }

#endif

    /// @}
    /// @name Topological filters
    /// @{

    template<typename P>
    [[nodiscard]] FeaturesBase membersOf(FeatureBase<P> feature) const
    {
        if(feature.isRelation()) return membersOf(RelationPtr(feature.ptr()));
        if(feature.isWay()) return nodesOf(WayPtr(feature.ptr()));
        return getEmpty();
    }

    template<typename P>
    [[nodiscard]] FeaturesBase nodesOf(FeatureBase<P> feature) const
    {
        if(feature.isWay()) return nodesOf(WayPtr(feature.ptr()));
        return getEmpty();
    }

    template<typename P>
    [[nodiscard]] FeaturesBase parentsOf(FeatureBase<P> feature) const
    {
        if (feature.isAnonymousNode())
        {
            return parentWaysOf(feature.xy());
        }
        return parentsOf(FeaturePtr(feature.ptr()));
    }

    /// @brief Only relation members with the given role.
    ///
    /// @param role
    ///
    [[nodiscard]] FeaturesBase withRole(const std::string_view role) const
    {
        return withRole(std::span(&role, 1));
    }

    /// @brief Only relation members with one of the given roles.
    ///
    /// @param roles
    ///
    [[nodiscard]] FeaturesBase withRole(std::span<const std::string_view> roles) const
    {
        return withFilter(Filters::withRole(roles, store()->strings()));
    }

    /// @brief Only relation members with one of the given roles.
    ///
    /// Example:
    ///
    /// @code
    /// busRoute.members().withRole("forward", "backward")
    /// @endcode
    ///
    /// @param roles
    ///
    template <typename... Strings>
    requires (sizeof...(Strings) >= 2 &&
        (std::is_convertible_v<Strings, const char*> && ...))
    [[nodiscard]] FeaturesBase withRole(Strings&&... roles)
    {
        // Converts zero or more null-terminated strings and delegates to withRole().
        std::array<std::string_view, sizeof...(Strings)> roleStrings
        {
            std::string_view{std::forward<Strings>(roles)}...
        };
        return withRole(std::span<std::string_view>{roleStrings});
    }

    /// @}

    template <typename Predicate>
    [[nodiscard]] FeaturesBase filter(Predicate predicate) const
    {
        return withFilter(new PredicateFilter<Predicate>(predicate));
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

protected:
    FeaturesBase(const View& view) : View(view)
    {
    }

    FeatureIterator<T> query() const;

    static View rootView(const char* golFile)
    {
        FeatureStore* store = FeatureStore::openSingle(golFile);
        const MatcherHolder* matcher = store->getAllMatcher();
        return View(View::WORLD, 0, FeatureTypes::ALL, store,
            Box::ofWorld(), matcher, nullptr);
    }

    // FeaturesBase withFilter(PreparedFilterFactory& factory, Feature feature) const;

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

