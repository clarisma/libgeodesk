// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <map>
#include <unordered_map>
#include <vector>
#include <geodesk/feature/forward.h>
#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/feature/TagIterator.h>
#include <geodesk/feature/Tag.h>

namespace geodesk {

/// @brief An object describing the key/value attributes
/// of a Feature.
/// Tags are obtained via Feature::tags() and can be
/// iterated or implicitly converted to a `std::vector`,
/// `std::map` or `std::unordered_map`.
///
/// ```
/// Tags tags = hotel.tags();
/// for(Tag tag: tags)
/// {
///     std::cout << tag.key() << " = " << tag.value() << std::endl;
/// }
/// ```
///
/// **Note:** By default, tags are returned in *storage order*
/// of their keys, which is consistent across all tag sets stored in
/// the same GOL, but which generally is not *alphabetical order*.
/// If you want tags sorted alphabetically by key, assign the
/// Tags object to a `std::map`, or to a `std::vector` which you
/// can then explicitly `std::sort()`.
///
/// **Warning**: A Tags object is a lightweight wrapper
/// around a pointer to a Feature's tag data, contained
/// in a FeatureStore. It becomes invalid once that
/// FeatureStore is closed. To use a Feature's keys and
/// values beyond the lifetime of the FeatureStore,
/// assign the Tags object to a `std::vector` or
/// `std::unordered_map` whose types are `std::string`,
/// which allocate copies of the key/value data.
///
/// @see Tag, TagValue
///
class GEODESK_API Tags
{
public:
    explicit Tags(const Feature& feature);
    Tags(FeatureStore* store, FeaturePtr ptr) :
        store_(store), tags_(ptr.tags()) {}
    Tags(FeatureStore* store, TagTablePtr ptr) :
        store_(store), tags_(ptr) {}


    Tags(const Tags& other) = default;

    // TODO
    class Iterator
    {
    public:
        // NOLINTNEXTLINE key_ & value_ initialized by fetchNext()
        explicit Iterator(const Tags& tags) :
            iter_(tags.tags_, tags.store_->strings())
        {
            fetchNext();
        }

        Tag operator*() const
        {
            TagValue v = iter_.tags().tagValue(value_, iter_.strings());
            return { StringValue(key_), v };
        }
        Iterator& operator++()
        {
            fetchNext();
            return *this;
        }

        bool operator!=(std::nullptr_t) const
        {
            return key_ != nullptr;
        }

    private:
        void fetchNext()
        {
            std::pair<const clarisma::ShortVarString*, TagBits> tag = iter_.next();
            key_ = tag.first;
            value_ = tag.second;
        }

        TagIterator iter_;
        const clarisma::ShortVarString* key_;
        TagBits value_;
    };

    Iterator begin() const { return Iterator(*this); }
    static std::nullptr_t end() { return nullptr; }  // Simple sentinel

    /// @brief Returns the number of tags.
    ///
    [[nodiscard]] size_t size() const noexcept
    {
        return tags_.count();
    }

    /// @brief Looks up the tag value for the given key.
    ///
    /// *Note*: To efficiently look up the same tag for multiple
    ///  features in the same GOL, consider using a Key.
    ///
    /// @return the tag's value (or an empty string
    ///         if no tag with this key exists)
    TagValue operator[](std::string_view key) const noexcept
    {
        const TagBits val = tags_.getKeyValue(key, store_->strings());
        return tags_.tagValue(val, store_->strings());
    }

    /// @brief Looks up the tag value for the given key.
    ///
    /// @return the tag's value (or an empty string
    ///         if no tag with this key exists)
    TagValue operator[](Key key) const noexcept
    {
        const TagBits val = tags_.getKeyValue(key);
        return tags_.tagValue(val, store_->strings());
    }

    /// @brief Checks if this set of tags contains
    /// a tag with the given key.
    ///
    /// *Note*: To efficiently check the same tag for multiple
    ///  features in the same GOL, consider using a Key.
    ///
    [[nodiscard]] bool hasTag(std::string_view k) const noexcept
    {
        return tags_.getKeyValue(k, store_->strings()) != 0;
    }

    /// @brief Checks if this set of tags contains
    /// a tag with the given key.
    ///
    [[nodiscard]] bool hasTag(Key key) const noexcept
    {
        return tags_.getKeyValue(key) != 0;
    }

    /// @brief Checks if this set of tags contains
    /// a tag with the given key and value.
    ///
    [[nodiscard]] bool hasTag(std::string_view k, std::string_view v) const noexcept;

    /// @brief Checks if this set of tags contains
    /// a tag with the given key and value.
    ///
    [[nodiscard]] bool hasTag(Key k, std::string_view v) const noexcept;

    /// @brief Creates a map of keys to values. Tags are sorted
    /// in alphabetical order of their keys.
    ///
    /// @tparam K  The key's type: StringValue, `std::string` or `std::string_view`
    /// @tparam V  The value's type: TagValue, `std::string`, `double`, `int` or `bool`
    ///
    template<typename K, typename V>
    operator std::map<K,V>() const // NOLINT(google-explicit-constructor)
    {
        std::map<K,V> map;
        for(Tag tag: *this)
        {
            map.insert({ tag.key(), tag.value() });
        }
        return map;
    }

    /// @brief Creates an unordered map of keys to values.
    ///
    /// @tparam K  The key's type: StringValue, `std::string` or `std::string_view`
    /// @tparam V  The value's type: TagValue, `std::string`, `double`, `int` or `bool`
    ///
    template<typename K, typename V>
    operator std::unordered_map<K,V>() const // NOLINT(google-explicit-constructor)
    {
        std::unordered_map<K,V> map;
        for(Tag tag: *this)
        {
            map.insert({ tag.key(), tag.value() });
        }
        return map;
    }

    /// @brief Creates a vector containing the key/value pairs
    ///
    /// @param T  Tag or `std::pair<K,V>` (where `K` can be
    ///    StringValue, `std::string` or `std::string_view`,
    ///    and `V` can be TagValue, `std::string`, `double`,
    ///    `int` or `bool`)
    ///
    template<typename T>
    operator std::vector<T>() const // NOLINT(google-explicit-constructor)
    {
        std::vector<T> list;
        for(Tag tag: *this)
        {
            list.emplace_back(tag);
        }
        return list;
    }

    /// Checks if both Tags objects contain the same tags.
    ///
    bool operator==(const Tags& other) const;
    bool operator!=(const Tags& other) const
    {
        return !(*this == other);
    }
    // may allocate, hence throw

private:
    FeatureStore* store_;
    TagTablePtr tags_;
};

} // namespace geodesk
