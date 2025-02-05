// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <cstdint>
#include <clarisma/util/DataPtr.h>
#include <geodesk/feature/FeatureTypes.h>
#include <geodesk/feature/NodePtr.h>
#include <geodesk/feature/WayPtr.h>
#include <geodesk/feature/RelationPtr.h>
#include <geodesk/feature/TileConstants.h>
#include <geodesk/feature/TilePtr.h>

namespace geodesk {

using namespace TileConstants;

/// \cond lowlevel

template<typename Derived>
class SimpleQueryBase
{
protected:
    bool acceptTypes(FeatureTypes types) const  // CRTP virtual
    {
        return true;
    }

    bool acceptIndex(FeatureIndexType index, uint32_t keys) const  // CRTP virtual
    {
        return true;
    }

    bool acceptBounds(const Box& bounds) const    // CRTP virtual
    {
        return true;
    }

    bool acceptCoordinate(Coordinate xy) const    // CRTP virtual
    {
        return true;
    }

    void acceptNode(NodePtr node)               // CRTP virtual
    {
    }

    void acceptFeature(FeaturePtr feature)               // CRTP virtual
    {
    }

	void perform(const TilePtr pTile)
	{
        if(self().acceptType(FeatureTypes::NODES))
        {
		    searchNodes(pTile.ptr() + NODE_INDEX_OFS);
        }
        if(self().acceptType(FeatureTypes::NONAREA_WAYS))
        {
            searchFeatures(FeatureIndexType::WAYS, pTile.ptr() + WAY_INDEX_OFS);
        }
        if(self().acceptType(FeatureTypes::AREAS))
        {
            searchFeatures(FeatureIndexType::AREAS, pTile.ptr() + AREA_INDEX_OFS);
        }
        if(self().acceptType(FeatureTypes::NONAREA_RELATIONS))
        {
            searchFeatures(FeatureIndexType::RELATIONS, pTile.ptr() + RELATION_INDEX_OFS);
        }
	}

    void searchNodes(DataPtr ppIndex)
    {
        int32_t ptr = ppIndex.getInt();
        if (ptr == 0) return;
	    DataPtr p = ppIndex + ptr;
        for (;;)
        {
            ptr =  p.getInt();
            int32_t last = ptr & 1;
            int32_t keys = (p+4).getInt();
            if(acceptIndex(FeatureTypes::NODES, keys))
            {
                searchNodeBranch(p + (ptr ^ last));
            }
            if (last != 0) break;
            p += 8;
        }
    }

    void searchNodeBranch(DataPtr p)
    {
        for (;;)
        {
            int32_t taggedPtr = p.getInt();
            int last = taggedPtr & 1;
            if (acceptBounds(*reinterpret_cast<const Box*>(p.ptr() + 4)))
            {
                DataPtr pChild = p + (taggedPtr & 0xffff'fffc);
                if ((taggedPtr & 2) != 0)
                {
                    searchNodeLeaf(pChild);
                }
                else
                {
                    searchNodeBranch(pChild);
                }
            }
            if (last != 0) break;
            p += 20;
        }
    }

    void searchNodeLeaf(DataPtr p)
    {
        for (;;)
        {
            DataPtr pNode = p + 8;
            if (acceptCoordinate(Coordinate(p.getInt(), (p+4).getInt())))
            {
                self().acceptNode(NodePtr(pNode));
            }
            int flags = pNode.getInt();
            if ((flags & 1) != 0) break;
            p += 20 + (flags & 4);
            // If Node is member of relation (flag bit 2), add
            // extra 4 bytes for the relation table pointer
        }
    }

    void searchFeatures(FeatureIndexType indexType, DataPtr ppTree)
    {
        int32_t ptr = ppTree.getInt();
        if (ptr == 0) return;
	    DataPtr p = ppTree + ptr;
        for (;;)
        {
            ptr = p.getInt();
            int32_t last = ptr & 1;
            int32_t keys = (p+4).getInt();
            if (acceptIndex(indexType, keys))
            {
                searchBranch(p + (ptr ^ last));
            }
            if (last != 0) break;
            p += 8;
        }
    }

    void searchBranch(DataPtr p)
    {
        for (;;)
        {
            int32_t ptr = p.getInt();
            int32_t last = ptr & 1;
            if (acceptBounds(*reinterpret_cast<const Box*>(p.ptr() + 4)))
            {
                DataPtr pChild = p + (ptr & 0xffff'fffc);
                if (ptr & 2)
                {
                    searchLeaf(pChild);
                }
                else
                {
                    searchBranch(pChild);		// NOLINT recursion
                }
            }
            if (last != 0) break;
            p += 20;
        }
    }

    void searchLeaf(DataPtr p)
    {
        for (;;)
        {
            DataPtr pFeature = p + 16;
            if (acceptBounds(*reinterpret_cast<const Box*>(p.ptr())))
            {
                self().acceptFeature(FeaturePtr(pFeature));
            }
            int flags = pFeature.getInt();
            if ((flags & 1) != 0) break;
            p += 32;
        }
    }

private:
    Derived& self()
    {
        return *static_cast<Derived*>(this);
    }
};

// \endcond
} // namespace geodesk
