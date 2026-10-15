// Copyright (c) 2026 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <cstdint>
#include <clarisma/util/DataPtr.h>
#include <geodesk/feature/NodePtr.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/feature/TileConstants.h>
#include <geodesk/feature/TilePtr.h>

namespace geodesk {

template<typename Derived>
class TileScannerBase
{
public:
	bool scanFeatures(const TilePtr pTile)
	{
	    if (self().acceptNodes())
	    {
	        if (scanNodeIndexes(pTile.ptr() + TileConstants::NODE_INDEX_OFS)) return true;
	    }
	    if (self().acceptLinearWays())
	    {
	        if (scanIndexes(pTile.ptr() + TileConstants::WAY_INDEX_OFS,
	            FeatureIndexType::WAYS))
	        {
	            return true;
	        }
	    }
	    if (self().acceptAreas())
	    {
	        if (scanIndexes(pTile.ptr() + TileConstants::AREA_INDEX_OFS,
	            FeatureIndexType::AREAS))
	        {
	            return true;
	        }
	    }
	    if (self().acceptNonAreaRelations())
	    {
	        if (scanIndexes(pTile.ptr() + TileConstants::RELATION_INDEX_OFS,
	            FeatureIndexType::RELATIONS))
	        {
	            return true;
	        }
	    }
	    return false;
	}

    Derived& self() noexcept
	{
	    return static_cast<Derived&>(*this);
	}

    const Derived& self() const noexcept
	{
	    return static_cast<const Derived&>(*this);
	}

private:
    bool scanNodeIndexes(DataPtr ppIndex)
    {
        int32_t ptr = ppIndex.getInt();
        if (ptr == 0) return false;
	    DataPtr p = ppIndex + ptr;
        for (;;)
        {
            ptr = p.getInt();
            int last = ptr & 1;
            int32_t keys = (p+4).getInt();
            if (self().acceptKeys(FeatureIndexType::NODES, keys))
            {
                if (scanNodeBranch(p + (ptr ^ last))) return true;
            }
            if (last != 0) break;
            p += 8;
        }
	    return false;
    }


    bool scanNodeBranch(DataPtr p)
    {
        for (;;)
        {
            int32_t ptr = p.getInt();
            int last = ptr & 1;
            if (self().acceptBranch(*reinterpret_cast<const Box*>((p+4).ptr())))
            {
                if ((ptr & 2) != 0)
                {
                    if (scanNodeLeaf(p + (ptr ^ 2 ^ last))) return true;
                }
                else
                {
                    if (scanNodeBranch(p + (ptr ^ last))) return true;
                }
            }
            if (last != 0) break;
            p += 20;
        }
	    return false;
    }

    bool scanNodeLeaf(DataPtr p)
    {
        p += 8;
        for (;;)
        {
            int flags = p.getInt();
            if (self().node(NodePtr(p))) return true;
            if ((flags & 1) != 0) break;
            p += 20 + (flags & 4);
            // If Node is member of relation (flag bit 2), add
            // extra 4 bytes for the relation table pointer
        }
	    return false;
    }

    bool scanIndexes(DataPtr ppTree, FeatureIndexType indexType)
    {
        int32_t ptr = ppTree.getInt();
        if (ptr == 0) return false;
	    DataPtr p = ppTree + ptr;
        for (;;)
        {
            ptr = p.getInt();
            int32_t last = ptr & 1;
            int32_t keys = (p+4).getInt();
            if (self().acceptKeys(indexType, keys))
            {
                if (scanBranch(p + (ptr ^ last))) return true;
            }
            if (last != 0) break;
            p += 8;
        }
	    return false;
    }

    bool scanBranch(DataPtr p)
    {
        for (;;)
        {
            int32_t ptr = p.getInt();
            int last = ptr & 1;
            if (self().acceptBranch(*reinterpret_cast<const Box*>((p+4).ptr())))
            {
                if ((ptr & 2) != 0)
                {
                    if (scanLeaf(p + (ptr ^ 2 ^ last))) return true;
                }
                else
                {
                    if (scanBranch(p + (ptr ^ last))) return true;
                }
            }
            if (last != 0) break;
            p += 20;
        }
	    return false;
    }

    bool scanLeaf(DataPtr p)
    {
        p += 16;
        for (;;)
        {
            int flags = p.getInt();
            if (self().wayOrRelation(FeaturePtr(p))) return true;
            if ((flags & 1) != 0) break;
            p += 32;
        }
	    return false;
    }

    // === CRTP virtual methods ===

    bool acceptNodes() const  // CRTP virtual
    {
        return self().acceptIndex(FeatureIndexType::NODES);
    }

    bool acceptLinearWays() const  // CRTP virtual
    {
        return self().acceptIndex(FeatureIndexType::WAYS);
    }

    bool acceptAreas() const  // CRTP virtual
    {
        return self().acceptIndex(FeatureIndexType::AREAS);
    }

    bool acceptNonAreaRelations() const  // CRTP virtual
    {
        return self().acceptIndex(FeatureIndexType::RELATIONS);
    }

    bool acceptIndex(FeatureIndexType indexType) const  // CRTP virtual
    {
        return true;
    }

    bool acceptKeys(FeatureIndexType indexType, int32_t keys) const // CRTP virtual
    {
        return true;
    }

    bool acceptBranch(const Box& bbox) const // CRTP virtual
    {
        return true;
    }

    /*
    bool acceptCoordinate(Coordinate xy) const // CRTP virtual
    {
        return true;
    }
    */

    bool node(NodePtr node)   // CRTP virtual
    {
        return false;
    }

    bool way(WayPtr way)   // CRTP virtual
    {
        return false;
    }

    bool relation(RelationPtr relation)   // CRTP virtual
    {
        return false;
    }

    bool wayOrRelation(FeaturePtr feature)   // CRTP virtual
    {
        if (feature.isWay()) [[likely]]
        {
            return self().way(WayPtr(feature));
        }
        return self().relation(RelationPtr(feature));
    }
};

} // namespace geodesk