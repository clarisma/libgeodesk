// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/geom/Box.h>

namespace geodesk
{
class Features;
class Nodes;
class Tags;
class TagValue;
}

class GEOSGeometry;
class GEOSContextHandle_t;
class OGRGeometry;

namespace geodesk {

///
/// @brief A geographic feature.
///
class Feature
{
public:
    Feature(FeatureStore* store, FeaturePtr ptr);

    /// @name Type & Identity
    /// @{

    /// @brief The Feature's type
    ///
    /// @return `NODE`, `WAY` or `RELATION`
    FeatureType type() const noexcept;

    /// @brief The ID of this Feature
    ///
    /// @return the ID of the Feature
    int64_t id() const noexcept;

    /// @brief `true` if this Feature is a @ref Node.
    ///
    bool isNode() const noexcept;

    /// @brief `true` if this Feature is an *anonymous node*.
    ///
    /// An anonymous node is a Node that has no tags and does not belong
    /// to any relation; it merely exists as a vertex of a Way.
    /// Its ID is always `0` and its `FeaturePtr` is `nullptr`.
    ///
    bool isAnonymousNode() const noexcept;

    /// @brief `true` if this Feature is a @ref Way.
    ///
    bool isWay() const noexcept;

    /// @brief `true` if this Feature is a @ref Relation.
    ///
    bool isRelation() const noexcept;

    /// @brief `true` if this Feature is an area (represented
    /// by either a @ref Way or @ref Relation).
    ///
    bool isArea() const noexcept;

    /// `true` if this Feature belongs to at least one Relation.
    bool belongsToRelation() const noexcept;

    /// @brief The Feature's role in its Relation, if it was returned
    /// via members() or Features::membersOf(), otherwise an empty string.
    ///
    /// @return the Feature's role (or an empty string)
    ///
    StringValue role() const noexcept;

    /// @brief Tests two features for equality.
    /// Two features are equal if they have the same type and ID.
    /// Anonymous nodes are equal if they are coincident.
    ///
    bool operator==(const Feature& other) const noexcept;
    bool operator!=(const Feature& other) const noexcept;


    /// @}
    /// @name Display
    /// @{

    /// @brief `"node"`, `"way"` or `"relation"`
    ///
    /// @return pointer null-terminated constant string
    ///
    const char* typeName() const noexcept;

    /// @brief Formats the string representation (e.g.\ `"node/123456"`)
    /// of this Feature into the provided buffer.
    ///
    /// Appends a null-terminator at the end of the output, and returns
    /// a pointer to the null-terminator, allowing easy
    /// concatenation.
    ///
    /// @param buf pointer to a `char` array of sufficient size
    /// @return a pointer to the null-terminator of the string in `buf`
    ///
    char* format(char* buf) const noexcept;

    /// @brief Creates a `std::string` with the Feature's type and ID.
    ///
    /// @return a std::string (e.g. `"node/123456"`)
    ///
    std::string toString() const;

    /// @brief Creates a short descriptive label.
    ///
    /// This is usually a combination of the type and name, such as
    /// `hotel "Four Seasons"`.
    ///
    std::string label() const;

    /// @}
    /// @name Tags
    /// @{

    /// @brief Obtains the tag value for the given key.
    ///
    /// *Note*: To efficiently look up the same tag for multiple
    ///  features in the same GOL, consider using a Key.
    ///
    /// @return the tag's value (or an empty string
    ///         if the feature doesn't have a tag with this key)
    TagValue operator[](std::string_view key) const noexcept;

    /// @brief Obtains the tag value for the given key.
    ///
    /// @return the tag's value (or an empty string
    ///         if the feature doesn't have a tag with this key)
    TagValue operator[](Key key) const noexcept;

    /// @brief A collection of the Feature's tags.
    ///
    Tags tags() const noexcept;

    /// @brief Checks if this Feature has a tag with the given key
    ///
    bool hasTag(std::string_view k) const noexcept;

    /// @brief Checks if this Feature has a tag with the given key
    ///
    bool hasTag(Key k) const noexcept;

    /// @brief Checks if this Feature has a tag with the
    /// given key and value.
    ///
    bool hasTag(std::string_view k, std::string_view v) const noexcept;

    /// @brief Checks if this Feature has a tag with the
    /// given key and value.
    ///
    bool hasTag(Key k, std::string_view v) const noexcept;

    /// @}
    /// @name Geometry
    /// @{

    /// @brief The bounding box of this Feature.
    ///
    /// @returns The Feature's bounding box (in Mercator projection)
    ///
    Box bounds() const noexcept;

    /// @brief The Mercator-projected x/y coordinate of a Node,
    /// or the center point of the bounding box for a Way or Relation.
    ///
    Coordinate xy() const noexcept;

    /// @brief The Mercator-projected x-coordinate of a Node,
    /// or the horizontal midpoint of the bounding box for a Way or Relation.
    ///
    int32_t x() const noexcept;

    /// @brief The Mercator-projected y-coordinate of a Node,
    /// or the vertical midpoint of the bounding box for a Way or Relation.
    ///
    int32_t y() const noexcept;

    /// @brief The longitude of this feature (in WGS-84 degrees).
    ///
    /// For a Way or Relation, this is the horizontal midpoint of
    /// its bounding box.
    ///
    double lon() const noexcept;

    /// @brief The latitude of this feature (in WGS-84 degrees).
    ///
    /// For a Way or Relation, this is the vertical midpoint of
    /// its bounding box.
    ///
    double lat() const noexcept;

    /// Computes the centroid of this Feature
    ///
    /// @return the Feature's centroid (in Mercator projection)
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    Coordinate centroid() const;

    /// @brief Measures the area of this Feature.
    ///
    /// @return area (in square meters), or `0` if the feature is not polygonal
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    double area() const;

    /// @brief Measures the length of this Feature.
    ///
    /// @return length (in meters), or `0` if the feature is not lineal
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    ///
    double length() const;


    /// @brief Creates a `GEOSGeometry` based on this Feature's geometry.
    /// Coordinates will be in Mercator projection. The caller assumes
    /// ownership of the newly created `GEOSGeometry` and is responsible
    /// for its cleanup.
    ///
    /// This method is only available if build option `GEODESK_WITH_GEOS`
    /// is enabled (off by default).
    ///
    /// @param geosContext The GEOS context o use
    ///
    /// @returns the pointer to the newly-created `GEOSGeometry`
    ///
    GEOSGeometry* toGeometry(GEOSContextHandle_t geosContext) const;

    /// @brief Creates an `OGRGeometry` based on this Feature's geometry.
    /// Coordinates will be in EPSG:4326 (WGS-84 longitude/latitude).
    /// The caller assumes ownership of the newly created `OSMGeometry`
    /// and is responsible for its cleanup.
    ///
    /// This method is only available if build option `GEODESK_WITH_OGR`
    /// is enabled (off by default).
    ///
    /// @returns the pointer to the newly-created `OGRGeometry`
    ///
    OGRGeometry* toOgrGeometry() const;

    /// @}
    /// @name Related Features
    /// @{

    /// @brief The nodes of this Way.
    ///
    /// @return an ordered collection of Node objects
    ///
    Nodes nodes() const;

    /// @brief The nodes of this Way that match the given query.
    ///
    /// @param query a query in <a href="https://docs.geodesk.com/goql">GOQL</a> format
    /// @return an ordered collection of Node objects
    /// @throws QueryException if the query is malformed.
    ///
    Nodes nodes(const char* query) const;

    /// @brief The members of this Relation.
    ///
    /// @return an ordered collection of Feature objects
    ///
    Features members() const;

    /// @brief The members of this Relation that match the given query.
    ///
    /// @param query a query in <a href="https://docs.geodesk.com/goql">GOQL</a> format
    /// @return an ordered collection of Feature objects
    /// @throws QueryException if the query is malformed.
    ///
    Features members(const char* query) const;

    /// @brief The parent relations of this Feature (and the parent ways
    /// for a Node).
    ///
    /// @return a collection of Relation objects (Way and Relation for Node)
    ///
    Features parents() const;

    /// @brief The parent relations of this Feature (and the parent ways
    /// for a Node) that match the given query.
    ///
    /// @param query a query in <a href="https://docs.geodesk.com/goql">GOQL</a> format
    /// @return a collection of Relation objects (Way and Relation for Node)
    /// @throws QueryException if the query is malformed.
    ///
    Features parents(const char* query) const;

    /// @}
    /// @name Access to the Low-Level API
    /// @{

    /// @brief Pointer to the stored data
    /// of this Feature. If this Feature is an anonymous
    /// node, the `FeaturePtr` will be a `nullptr`
    ///
    FeaturePtr ptr() const noexcept;

    /// @brief Pointer to the FeatureStore
    /// which contains this Feature.
    ///
    FeatureStore* store() const noexcept;

    /// @}
};

} // namespace geodesk



