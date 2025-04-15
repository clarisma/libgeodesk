// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <vector>
#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/geom/Box.h>

namespace geodesk
{
class Feature;
class Nodes;
class Ways;
class Relations;
}

namespace geodesk {

///
/// @brief A collection of geographic features.
///
/// A Features object isn't a traditional container; it doesn't actually
/// hold any Feature objects. Instead, it defines the criteria for retrieving
/// features from a Geographic Object Library. A query is only
/// executed whenever the Features object is iterated, assigned to a container,
/// or when one of its scalar functions (such as count() or length()) is called.
/// Query results are not cached. For example, calling count() before
/// iterating over a Features object will result in the query being
/// executed twice.
///
/// Features objects are lightweight, thread-safe (see note below), and designed to be
/// passed by value. They can be safely shared across threads. Matchers,
/// filters, and related resources (such as temporary indexes) use
/// reference counting to ensure efficient and safe resource management.
///
/// To start working with a GOL, simply create a `Features` object
/// with the path of its file (the `.gol` extension may be omitted):
///
/// ```
/// Features world("path/to/planet.gol");
/// ```
/// This creates a FeatureStore that manages access to `planet.gol`.
/// The `world` object now represents a collection of all features
/// stored in the GOL (It is legal to create multiple `Features`
/// objects that refer to the same GOL -- they will share the same
/// FeatureStore).
///
/// To obtain subsets, apply filters using `()` or by calling a
/// named method -- or intersect multiple `Features` object with `&`:
///
/// ```
/// Features hotels = world("na[tourism=hotel]");  // All hotels
/// Features inParis = world.within(paris);        // All features in Paris
///
/// // Only hotels in Paris:
/// Features hotelsInParis = hotels.within(paris);
/// // or...
/// Features hotelsInParis = hotels & inParis;
/// ```
///
/// Applying a filter creates a copy of the Features object, with
/// the additional constraint. The original Features object is unaffected.
///
/// To retrieve the actual features:
///
/// ```
/// // By iterating:
/// for(Feature hotel : hotelsInParis)
/// {
///     std::cout << hotel["name"] << std::endl;
/// }
///
/// // As a vector:
/// std::vector<Feature> hotelList = hotelsInParis;
///
/// // The one and only:
/// Feature paris = world("a[boundary=administrative]"
///      "[admin_level=8][name=Paris]").one();
///
/// // The first (if any):
/// std::optional<Feature> anyHotel = hotels.first();
/// ```
///
/// Features has three subclasses: Nodes, Ways and Relations, which contain
/// only Node, Way and Relation objects. Assigning a Features object to a
/// subclass object implicitly filters the collection by type (Assigning
/// a subtype to an incompatible type results in an empty collection):
///
/// ```
/// Features world("world");
///
/// Relations busRoutes =                  // Only relations that
///     world("[type=route][route=bus]");  // are tagged as bus routes
///
/// Nodes nodes = busRoutes;               // empty collection
/// ```
///
/// When the last Features object referring to a FeatureStore is destroyed,
/// the associated Geographic Object Library is automatically closed,
/// and all resources managed by the FeatureStore are released. (This differs
/// from [GeoDesk for Java](https://docs.geodesk.com/java/libraries#closing-a-library),
/// which requires GOLs to be closed explicitly).
///
/// @warning
/// After a GOL has been closed, any Feature objects obtained
/// from it become invalid. Calling their methods results in undefined behavior.
/// This restriction also applies to related types, such as Tags, Tag, TagValue
/// and StringValue.
///
/// @note
/// To allow multiple threads to share Features objects that use the same
/// GOL, GeoDesk must be built with option `GEODESK_MULTITHREADED` enabled.
/// Leaving this option disabled may improve the performance of single-threaded
/// applications.
///
class Features
{
public:
    /// @brief Creates a collection that contains all features in the
    /// given Geographic Object Library
    ///
    /// @param golFile path of the GOL (`.gol` extension may be omitted)
    ///
    Features(const char* golFile);

    /// @brief Creates a collection with all the features in
    /// the other collection.
    ///
    Features(const Features& other);

    /// @{
    /// @name State and Membership

    /// @brief Returns `true` if this collection contains
    /// at least one Feature.
    ///
    operator bool() const;

    /// @brief Returns `true` if this collection is empty.
    ///
    bool operator!() const;

    /// @brief Checks if the specified Feature exists in this collection.
    ///
    bool contains(const Feature& feature) const;

    /// @}
    /// @{
    /// @name Filtering by type and tags

    /// @brief Only features that match the given query.
    ///
    /// @param query a query in <a href="https://docs.geodesk.com/goql">GOQL</a> format
    ///
    /// @throws QueryException if the query is malformed.
    ///
    Features operator()(const char* query) const;

    ///
    /// @brief Only nodes.
    ///
    Nodes nodes() const;

    ///
    /// @brief Only nodes that match the given query.
    ///
    /// @param query a query in <a href="https://docs.geodesk.com/goql">GOQL</a> format
    ///
    /// @throws QueryException if the query is malformed.
    ///
    Nodes nodes(const char* query) const;

    ///
    /// @brief Only ways.
    ///
    Ways ways() const;

    ///
    /// @brief Only ways that match the given query.
    ///
    /// @param query a query in <a href="https://docs.geodesk.com/goql">GOQL</a> format
    ///
    /// @throws QueryException if the query is malformed.
    ///
    Ways ways(const char* query) const;

    ///
    /// @brief Only relations.
    ///
    Relations relations() const;

    ///
    /// @brief Only relations that match the given query.
    ///
    /// @param query a query in <a href="https://docs.geodesk.com/goql">GOQL</a> format
    ///
    /// @throws QueryException if the query is malformed.
    ///
    Relations relations(const char* query) const;

    /// @}
    /// @name Retrieving Features
    /// @{

    /// @brief Returns the first Feature in this collection, or `std::nullopt` if the
    /// collection is empty.
    ///
    /// Only Nodes of a Way and member Features of a Relation are ordered; for all
    /// other collections, first() returns an arbitrary Feature.
    ///
    /// @return the first `Feature`, or `std::nullopt`
    ///
    std::optional<Feature> first() const;

    /// @brief Returns the one and only Feature in this collection.
    ///
    /// @return the sole Feature
    ///
    /// @throws QueryException if this collection is empty or contains more
    /// than one Feature
    ///
    Feature one() const;

    /// @brief Returns a `std::vector` with the Feature objects in this collection.
    ///
    operator std::vector<Feature>() const;

    /// @brief Returns a `std::vector` with the FeaturePtr pointers that
    /// refer to the Feature objects in this collection.
    ///
    /// @fn geodesk::Features::operator std::vector<geodesk::FeaturePtr>() const
    operator std::vector<FeaturePtr>() const;

    /// @brief Appends the Feature objects in this collection to the given `std::vector`.
    ///
    void addTo(std::vector<Feature>& features) const;

    /// @brief Appends the Feature objects in this collection to the given `std::vector`
    /// as FeaturePtr pointers.
    ///
    void addTo(std::vector<FeaturePtr>& features) const;

    /// @}
    /// @name Scalar Queries
    /// @{

    /// @brief Returns the total number of features in this collection.
    ///
    uint64_t count() const;

    /// @brief Computes the total length (in meters) of the features
    /// in this collection.
    ///
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    double length() const;

    /// @brief Computes the total area (in square meters) of the features
    /// in this collection.
    ///
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    double area() const;

    /// @}
    /// @name Spatial Filters
    /// @{

    /// @brief Only features whose bounding box intersects
    /// the given bounding box.
    ///
    /// @param box
    Features operator()(const Box& box) const;

    /// @brief Only features whose bounding box contains
    /// the given Coordinate.
    ///
    /// @param xy
    Features operator()(Coordinate xy) const;

    /// @brief Only features whose geometry intersects with the
    /// given Feature (short form of intersecting())
    ///
    Features operator()(const Feature& feature) const;

    /// @brief Only features whose geometry intersects with the
    /// given Feature.
    ///
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    Features intersecting(const Feature& feature) const;

    /// @brief Only features that lie entirely inside the geometry
    /// of the given Feature.
    ///
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    Features within(const Feature& feature) const;

    /// @brief Only features whose geometry contains the
    /// given Feature.
    ///
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    Features containing(const Feature& feature) const;

    /// @brief Only features whose geometry contains the
    /// given Coordinate.
    ///
    Features containing(Coordinate xy) const;

    /// @brief Only features whose geometry contains the
    /// given location.
    ///
    /// @param lon degrees longitude
    /// @param lat degrees latitude
    ///
    Features containingLonLat(double lon, double lat) const;

    /// @brief Only features whose geometry crosses the
    /// given Feature.
    ///
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    Features crossing(const Feature& feature) const;

    /// @brief Only features whose closest point lies within
    /// `distance` meters of `xy`.
    ///
    /// @param distance the maximum distance (in meters)
    /// @param xy the center of the search radius
    ///
    Features maxMetersFrom(double distance, Coordinate xy) const;

    /// @brief Only features whose closest point lies within
    /// `distance` meters of the given location.
    ///
    /// @param distance the maximum distance (in meters)
    /// @param lon  longitude of the search radius center
    /// @param lat  latitude of the search radius center
    ///
    Features maxMetersFrom(double distance, double lon, double lat) const;

    /// @}
    /// @name Topological Filters
    /// @{

    /// @brief Only nodes that belong to the given Way.
    ///
    Features nodesOf(const Feature& feature) const;

    /// @brief Only features that belong to the given Relation.
    ///
    Features membersOf(const Feature& feature) const;

    /// @brief Only features that are parent relations of the
    /// given Feature (or parent ways of the given Node).
    ///
    Features parentsOf(const Feature& feature) const;

    /// @brief Only features that share a common node with
    /// the given Feature.
    ///
    Features connectedTo(const Feature& feature) const;

    /// @}
    /// @name Filtering with Predicate
    /// @{

    /// @brief Only features that match the given predicate.
    ///
    /// @param predicate A callable object (e.g., lambda,
    ///  function pointer, or functor) that defines the
    ///  filtering logic. The callable must accept a
    ///  Feature and return a `bool`.
    ///
    /// **Important:** The provided predicate must be
    /// thread-safe, as it may be invoked concurrently.
    ///
    /// ```
    /// // Find all parks whose area is at least 1 km²
    /// // (one million square meters)
    ///
    /// Features parks = world("a[leisure=park]");
    /// Features largeParks = parks.filter([](Feature park)
    ///     { return park.area() > 1'000'000; });
    /// ```
    template <typename Predicate>
    Features filter(Predicate predicate) const;

    /// @}
    /// @name Metadata
    /// @{

    /// @brief Obtains a Key for the given string, which can be
    /// used for faster tag-value lookups.
    ///
    /// **Important:** The resulting Key can only be used for
    /// features that are stored in the same GOL.
    ///
    Key key(std::string_view k) const;

    /// @}
    /// @name Access to the Low-Level API
    /// @{

    /// @brief Returns a pointer to the FeatureStore
    /// which contains the features in this collection.
    ///
    FeatureStore* store() const noexcept;

    /// @}
};

/// @brief A collection containing only Node features.
///
class Nodes : public Features
{
};

/// @brief A collection containing only Way features.
///
class Ways : public Features
{
};

/// @brief A collection containing only Relation features.
///
class Relations : public Features
{
};

} // namespace geodesk



