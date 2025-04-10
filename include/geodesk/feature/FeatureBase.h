// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <clarisma/util/TaggedPtr.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/feature/FeatureUtils.h>
#include <geodesk/feature/Tags.h>
#include <geodesk/feature/NodePtr.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/geom/Area.h>
#include <geodesk/geom/Box.h>
#include <geodesk/geom/Centroid.h>
#include <geodesk/geom/Length.h>
#include <geodesk/geom/Mercator.h>

#ifdef GEODESK_WITH_GEOS
#include <geos_c.h>
#endif

namespace geodesk {

/// \cond

class Features;
class Nodes;
template <typename T>
class FeatureIterator;

///
/// Do not show this class.
///
template<typename T>
class FeatureBase
{
    static constexpr bool canBeAny = std::is_same_v<T, FeaturePtr>;
    static constexpr bool canBeNode =
        std::is_same_v<T, FeaturePtr> || std::is_same_v<T, NodePtr>;
    static constexpr bool canBeWay =
        std::is_same_v<T, FeaturePtr> || std::is_same_v<T, WayPtr>;
    static constexpr bool canBeRelation =
        std::is_same_v<T, FeaturePtr> || std::is_same_v<T, RelationPtr>;

    //using Feature_ = FeatureBase<FeaturePtr,true,true,true>;
    //using Node_ = FeatureBase<NodePtr,true,false,false>;

public:
    FeatureBase(FeatureStore* store, FeaturePtr ptr, const clarisma::ShortVarString* role = nullptr)
    {
        store_ = TaggedPtr<FeatureStore,3>(store, ptr.typeCode() << 1);
        feature_.ptr = T(ptr);
        feature_.role = role;
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    FeatureBase(const FeatureBase<FeaturePtr>& other)
    {
        if constexpr (!canBeAny)
        {
            // target accepts only specific type,
            // needs runtime checks
            if (canBeNode && !other.isNode()) wrongType(other);
            if (canBeWay && !other.isWay()) wrongType(other);
            if (canBeRelation && !other.isRelation()) wrongType(other);
        }
        copyFrom(other);
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    FeatureBase(const FeatureBase<NodePtr>& other)
    {
        checkAssignableFromNode();
        copyFrom(other);
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    FeatureBase(const FeatureBase<WayPtr>& other)
    {
        checkAssignableFromWay();
        copyFrom(other);
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    FeatureBase(const FeatureBase<RelationPtr>& other)
    {
        checkAssignableFromRelation();
        copyFrom(other);
    }

    /// @name Type & Identity
    /// @{

    [[nodiscard]] FeatureType type() const noexcept
    {
        return static_cast<FeatureType>(typeCode());
    }

    /// @brief Returns the ID of this Feature
    ///
    /// @return the ID of the Feature
    [[nodiscard]] int64_t id() const noexcept
    {
        return isAnonymousNode() ? anonymousNode_.id : feature_.ptr.id();
    }

    /// @brief Returns `true` if this Feature is a @ref Node.
    ///
    [[nodiscard]] bool isNode() const noexcept
    {
        return (canBeNode && (extendedType() & 6) == 0) || (!canBeWay && !canBeRelation);
    }

    [[nodiscard]] bool isAnonymousNode() const noexcept
    {
        return canBeNode && (extendedType() & 1);
    }

    /// @brief Returns `true` if this Feature is a @ref Way.
    ///
    [[nodiscard]] bool isWay() const noexcept
    {
        return (canBeWay && (extendedType() == 2)) || (!canBeNode && !canBeRelation);
    }

    /// @brief Returns `true` if this Feature is a @ref Relation.
    ///
    [[nodiscard]] bool isRelation() const noexcept
    {
        return (canBeRelation && (extendedType() == 4)) || (!canBeNode && !canBeWay);
    }

    /// @brief Returns `true` if this Feature is an area (represented
    /// by either a @ref Way or @ref Relation)
    ///
    [[nodiscard]] bool isArea() const noexcept
    {
        return (canBeWay || canBeRelation) && !isNode() && feature_.ptr.isArea();
    }

    [[nodiscard]] bool belongsToRelation() const noexcept
    {
        if(isAnonymousNode()) return false;
        return feature_.ptr.isRelationMember();
    }

    /// @brief If this Feature was returned by a call to @ref Feature::members()
    /// of a Relation, returns this Feature's role in that Relation.
    ///
    /// @return the Feature's role (or an empty string)
    [[nodiscard]] StringValue role() const noexcept
    {
        if(isAnonymousNode()) return {};
        return feature_.role;
    }

    bool operator==(const Feature& other) const noexcept
    {
        if(isAnonymousNode())
        {
            return other.isAnonymousNode() &&
                anonymousNode_.id == other.anonymousNode_.id &&
                anonymousNode_.xy == other.anonymousNode_.xy;
        }
        return !other.isAnonymousNode() &&
            feature_.ptr.idBits() == other.feature_.ptr.idBits();
    }

    bool operator!=(const Feature& other) const noexcept
    {
        return !(*this == other);
    }


    /// @}
    /// @name Display
    /// @{

    /// @brief Depending on the type, returns `node`, `way`, or `relation`
    ///
    /// @return pointer null-terminated constant string
    ///
    [[nodiscard]] const char* typeName() const noexcept
    {
        if (isNode()) return "node";
        if (isWay()) return "way";
        assert(isRelation());
        return "relation";
    }

    /// @brief Formats the string representation (e.g. `node/123456`)
    /// of this Feature into the provided buffer. Appends a
    /// null-terminator at the end of the output, and returns
    /// a pointer to the null-terminator, allowing easy
    /// concatenation.
    ///
    /// @param buf pointer to a `char` array of sufficient size
    /// @return a pointer to the null-terminator
    ///
    char* format(char* buf) const noexcept
    {
        return FeatureUtils::format(buf, typeName(), id());
    }

    [[nodiscard]] std::string toString() const
    {
        char buf[32];
        format(buf);
        return {buf};
    }

    std::string label() const
    {
        return FeatureUtils::label(tags());
    }

    /// @}
    /// @name Tags
    /// @{

    /// @brief Obtains the tag value for the given key.
    ///
    /// @return the tag's value (or an empty string
    ///         if the feature doesn't have a tag with this key)
    TagValue operator[](const std::string_view& key) const noexcept
    {
        if(isAnonymousNode()) return {};     // empty string
        const TagTablePtr tags = feature_.ptr.tags();
        const TagBits val = tags.getKeyValue(key, store_.ptr()->strings());
        return tags.tagValue(val, store_.ptr()->strings());
    }

    /// @brief Obtains the tag value for the given key.
    ///
    /// @return the tag's value (or an empty string
    ///         if the feature doesn't have a tag with this key)
    TagValue operator[](Key key) const noexcept
    {
        if(isAnonymousNode()) return {};     // empty string
        const TagTablePtr tags = feature_.ptr.tags();
        const TagBits val = tags.getKeyValue(key);
        return tags.tagValue(val, store_.ptr()->strings());
    }

    /// @brief Checks if this set of tags contains
    /// a tag with the given key.
    ///
    [[nodiscard]] bool hasTag(std::string_view k) const noexcept
    {
        if(isAnonymousNode()) return false;
        return feature_.ptr.tags().getKeyValue(k, store()->strings()) != 0;
    }

    /// @brief Checks if this set of tags contains
    /// a tag with the given key.
    ///
    [[nodiscard]] bool hasTag(Key key) const noexcept
    {
        if(isAnonymousNode()) return false;
        return feature_.ptr.tags().getKeyValue(key) != 0;
    }

    [[nodiscard]] Tags tags() const noexcept
    {
        if(isAnonymousNode()) [[unlikely]]
        {
            return Tags(store());
        }
        return Tags(store(), ptr());
    }

    /// @}
    /// @name Geometry
    /// @{

    /// @brief Returns the bounding box of this Feature.
    ///
    /// @box The Feature's bounding box (in Mercator projection)
    ///
    [[nodiscard]] Box bounds() const noexcept
    {
        if (isNode())
        {
            return isAnonymousNode() ? Box(anonymousNode_.xy) : NodePtr(feature_.ptr).bounds();
        }
        return feature_.ptr.bounds();   // TODO: make Feature2DPtr
    }

    /// @brief Returns the Mercator-projected x/y coordinate of a Node,
    /// or the center point of the bounding box for a Way or Relation.
    ///
    [[nodiscard]] Coordinate xy() const noexcept
    {
        if (isNode())
        {
            return isAnonymousNode() ? anonymousNode_.xy : NodePtr(feature_.ptr).xy();
        }
        return feature_.ptr.bounds().center();   // TODO: make Feature2DPtr
    }

    /// @brief Returns the Mercator-projected x-coordinate of a Node,
    /// or the horizontal midpoint of the bounding box for a Way or Relation.
    ///
    [[nodiscard]] int32_t x() const noexcept { return xy().x; }

    /// @brief Returns the Mercator-projected y-coordinate of a Node,
    /// or the vertical midpoint of the bounding box for a Way or Relation.
    ///
    [[nodiscard]] int32_t y() const noexcept { return xy().y; }

    [[nodiscard]] double lon() const noexcept
    {
        return Mercator::roundedLonFromX(x());
    }

    [[nodiscard]] double lat() const noexcept
    {
        return Mercator::roundedLatFromY(y());
    }

    /// Calculates the centroid of this Feature
    ///
    /// @return the Feature's centroid (in Mercator projection)
    /// @throws QueryException if one or more tiles that contain
    ///   the geometry of a Relation are missing
    [[nodiscard]] Coordinate centroid() const
    {
        if (isNode()) return xy();
        if (isWay()) return Centroid::ofWay(WayPtr(feature_.ptr));
        assert(isRelation());
        return Centroid::ofRelation(store_.ptr(), RelationPtr(feature_.ptr));
    }

    /// @brief Measures the area of a feature
    ///
    /// @return area (in square meters), or `0` if the feature is not polygonal
    [[nodiscard]] double area() const
    {
        if(!isArea()) return 0;
        if(isWay()) return Area::ofWay(WayPtr(ptr()));
        assert(isRelation());
        return Area::ofRelation(store(), RelationPtr(ptr()));
    }

    /// @brief Measures the length of a feature.
    ///
    /// @return length (in meters), or `0` if the feature is not lineal
    [[nodiscard]] double length() const
    {
        if(isWay()) return Length::ofWay(WayPtr(ptr()));
        if(isRelation()) return Length::ofRelation(store(), RelationPtr(ptr()));
        return 0;
    }

    #ifdef GEODESK_WITH_GEOS
    [[nodiscard]] GEOSGeometry* toGeometry(GEOSContextHandle_t geosContext) const;
    #endif

    /// @}
    /// @name Related Features
    /// @{

    [[nodiscard]] Nodes nodes() const;
    [[nodiscard]] Nodes nodes(const char* query) const;
    [[nodiscard]] Features members() const;
    [[nodiscard]] Features members(const char* query) const;
    [[nodiscard]] Features parents() const;
    [[nodiscard]] Features parents(const char* query) const;

    /// @}
    /// @name Access to the Low-Level API
    /// @{

    [[nodiscard]] T ptr() const noexcept
    {
        return isAnonymousNode() ? T() : T(feature_.ptr);
    }

    [[nodiscard]] FeatureStore* store() const noexcept
    {
        return store_.ptr();
    }

    /// @}

private:
    enum class ExtendedFeatureType
    {
        NODE = 0,
        ANONYMOUS_NODE = 1,
        WAY = 2,
        RELATION = 4
    };

    // only used by FeatureIterator
    explicit FeatureBase(FeatureStore* store)
    {
        store_ = TaggedPtr<FeatureStore,3>(store);
        feature_.ptr = nullptr;
        feature_.role = nullptr;
    }

    bool isNull() const noexcept { return store_.rawPtr() == nullptr; }
    void setNull() noexcept { store_ = TaggedPtr<FeatureStore,3>(nullptr); }

    void setTypedFeature(FeaturePtr ptr) noexcept
    {
        store_.setFlags(ptr.typeCode() << 1);
        feature_.ptr = ptr;
    }

    void setType(ExtendedFeatureType type) noexcept
    {
        store_.setFlags(static_cast<int>(type));
    }

    void setFeature(FeaturePtr ptr) noexcept
    {
        feature_.ptr = ptr;
    }

    void setIdAndXY(int64_t id, Coordinate xy) noexcept
    {
        anonymousNode_.id = id;
        anonymousNode_.xy = xy;
    }

    void setRole(StringValue role) noexcept
    {
        feature_.role = role;
    }

    int typeCode() const { return store_.flags() >> 1; }
    int extendedType() const { return store_.flags(); }

    // Helper function to perform static assertions only when necessary.
    template<bool Cond = !canBeAny, std::enable_if_t<Cond, int> = 0>
    void checkAssignableFromNode()
    {
        static_assert(!canBeWay, "Can't assign Node to Way");
        static_assert(!canBeRelation, "Can't assign Node to Relation");
    }

    template<bool Cond = !canBeAny, std::enable_if_t<Cond, int> = 0>
    void checkAssignableFromWay()
    {
        static_assert(!canBeNode, "Can't assign Way to Node");
        static_assert(!canBeRelation, "Can't assign Way to Relation");
    }

    template<bool Cond = !canBeAny, std::enable_if_t<Cond, int> = 0>
    void checkAssignableFromRelation()
    {
        static_assert(!canBeNode, "Can't assign Relation to Node");
        static_assert(!canBeWay, "Can't assign Relation to Way");
    }

    // Dummy function to avoid issues when Cond is false
    void checkAssignableFromNode() {}
    void checkAssignableFromWay() {}
    void checkAssignableFromRelation() {}


    template <typename OtherT>
    void wrongType(const FeatureBase<OtherT>& other)
    {
        throw std::runtime_error(std::string("Attempt to assign ") +
            other.typeName() + " to " + typeName());
    }

    template <typename OtherT>
    void copyFrom(const FeatureBase<OtherT>& other)
    {
        store_ = other.store_;
        if (canBeNode)
        {
            if (other.isAnonymousNode())
            {
                anonymousNode_.id = other.anonymousNode_.id;
                anonymousNode_.xy = other.anonymousNode_.xy;
            }
            else
            {
                feature_.ptr = other.feature_.ptr;
                feature_.role = other.feature_.role;
            }
        }
        else
        {
            assert(!other.isAnonymousNode());
            feature_.ptr = other.feature_.ptr;
            feature_.role = other.feature_.role;
        }
    }

    TaggedPtr<FeatureStore,3> store_;
    union
    {
        struct
        {
            FeaturePtr ptr;
            StringValue role;
        }
        feature_;
        struct  // NOLINT
        {
            int64_t id;         // TODO: may need to put xy in same place as ptr
            Coordinate xy;
        }
        anonymousNode_;
    };

    friend class FeatureBase<FeaturePtr>;
    friend class FeatureBase<NodePtr>;
    friend class FeatureBase<WayPtr>;
    friend class FeatureBase<RelationPtr>;
    template <typename P>
    friend class FeatureIterator;
    friend class FeatureIteratorBase;
};

template<typename Stream,typename T>
Stream& operator<<(Stream& out, const FeatureBase<T>& f)
{
    char buf[32];
    const char *p = f.format(buf);
    out.write(buf, p - buf);
    return out;
}

using Feature = FeatureBase<FeaturePtr>;
using Node = FeatureBase<NodePtr>;
using Way = FeatureBase<WayPtr>;
using Relation = FeatureBase<RelationPtr>;

// \endcond

} // namespace geodesk

