// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#include <geodesk/query/TileQueryTask.h>
#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/feature/TileConstants.h>
#include <geodesk/feature/types.h>
#include <geodesk/query/Query.h>

using namespace TileConstants;

namespace geodesk {

QueryResultsHeader QueryResults::EMPTY_HEADER = { EMPTY, DataPtr(), DEFAULT_BUCKET_SIZE };
QueryResults* const QueryResults::EMPTY = reinterpret_cast<QueryResults*>(&EMPTY_HEADER);

// TODO: perform type check prior to matcher

	// Check for acceptable type (way, relation, member, way-node, etc.)
	// The following is valid for Java, check for C++:
	// (No need for AND with 0x1f, as int-shift only considers lower 5 bits)
	// if (((1 << (flags >> 1)) & acceptedTypes) == 0) break;

void TileQueryTask::operator()()
{
	Tip tip = Tip(tipAndFlags_ >> 8);
	pTile_ = query_->store()->fetchTile(tip);
	uint32_t types = query_->types();

	// LOG("Scanning tile %06X", tip);

	if (types & FeatureTypes::NODES) searchNodeIndexes();
	if (types & FeatureTypes::NONAREA_WAYS) searchIndexes(FeatureIndexType::WAYS);
	if (types & FeatureTypes::AREAS) searchIndexes(FeatureIndexType::AREAS);
	if (types & FeatureTypes::NONAREA_RELATIONS) searchIndexes(FeatureIndexType::RELATIONS);
	query_->offer(results_);
}

void TileQueryTask::searchNodeIndexes()
{
	const MatcherHolder* matcher = query_->matcher();
	DataPtr ppRoot = pTile_ + NODE_INDEX_OFS;
	int32_t ptr = ppRoot.getInt();
	if (ptr == 0) return;

	DataPtr p = ppRoot + ptr;
	for (;;)
	{
		ptr =  p.getInt();
		int32_t last = ptr & 1;
		int32_t keys = (p+4).getInt();
		if (matcher->acceptIndex(FeatureIndexType::NODES, keys))
		{
			searchNodeBranch(p + (ptr ^ last));
		}
		if (last != 0) break;
		p += 8;
	}
}


void TileQueryTask::searchNodeBranch(DataPtr p)
{
	// LOG("Searching branch at %016X", p);
	Box box = query_->bounds();
	for (;;)
	{
		int32_t ptr = p.getInt();
		int32_t last = ptr & 1;
		if (box.intersects(*reinterpret_cast<const Box*>((const uint8_t *)p + 4)))
		{
			DataPtr pChild = p + (ptr & 0xffff'fffc);
			if (ptr & 2)
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

void TileQueryTask::searchNodeLeaf(DataPtr p)
{
	// LOG("Searching leaf at %016X", p);
	Box box = query_->bounds();
	FeatureTypes acceptedTypes = query_->types();
	const Matcher& matcher = query_->matcher()->mainMatcher();

	for (;;)
	{
		int32_t flags = (p+8).getInt();
		if (box.contains(p.getInt(), (p+4).getInt()))
		{
			if (acceptedTypes.acceptFlags(flags))
			{
				FeaturePtr pFeature(p + 8);
				if (matcher.accept(pFeature))
				{
					const Filter* filter = query_->filter();
					if (filter == nullptr || filter->accept(query_->store(),
						pFeature, fastFilterHint_))
					{
						// LOG("Found node/%llu", Feature::id(pFeature));
						addResult(static_cast<uint32_t>(pFeature.ptr() - pTile_));
					}
				}
			}
		}
		if (flags & 1) break;
		p += 20 + (flags & 4);	
		// If Node is member of relation (flag bit 2), add
		// extra 4 bytes for the relation table pointer
	}
}



void TileQueryTask::searchIndexes(FeatureIndexType indexType)
{
	const MatcherHolder* matcher = query_->matcher();
	DataPtr ppRoot = pTile_ + NODE_INDEX_OFS + indexType * 4;
	int32_t ptr = ppRoot.getInt();
	if (ptr == 0) return;

	DataPtr p = ppRoot + ptr;
	for (;;)
	{
		ptr  = p.getInt();
		int32_t last = ptr & 1;
		int32_t keys = (p+4).getInt();
		if (matcher->acceptIndex(indexType, keys))
		{
			searchBranch(p + (ptr ^ last));
		}
		if (last != 0) break;
		p += 8;
	}
}


void TileQueryTask::searchBranch(DataPtr p)
{
	Box box = query_->bounds();
	for (;;)
	{
		int32_t ptr = p.getInt();
		int32_t last = ptr & 1;
		if (box.intersects(*reinterpret_cast<const Box*>((const uint8_t*)p + 4)))
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


void TileQueryTask::searchLeaf(DataPtr p)
{
	Box box = query_->bounds();
	FeatureTypes acceptedTypes = query_->types();
	const Matcher& matcher = query_->matcher()->mainMatcher();
	int multiTileFlags = tipAndFlags_ & (FeatureFlags::MULTITILE_NORTH | FeatureFlags::MULTITILE_WEST);
	for (;;)
	{
		int32_t flags = (p+16).getInt();
		if((flags & multiTileFlags) == 0)
		{
			if (!(p.getInt() > box.maxX() ||
				(p+4).getInt() > box.maxY() ||
				(p+8).getInt() < box.minX() ||
				(p+12).getInt() < box.minY()))
			{
				// TODO: replace this branching code with arithmetic?
				// Useful? https://stackoverflow.com/a/62852710

				if (acceptedTypes.acceptFlags(flags))
				{
					FeaturePtr pFeature (p + 16);
					if (matcher.accept(pFeature))
					{
						const Filter* filter = query_->filter();
						if (filter == nullptr || filter->accept(query_->store(), 
							pFeature, fastFilterHint_))
						{
							addResult(static_cast<uint32_t>(pFeature.ptr() - pTile_));
						}
					}
				}
			}
		}
		if (flags & 1) break;
		p += 32;
	}
}

/**
 * Add a relative pointer to the list of results.
 * If the current bucket is full, place a new bucket at the end
 * of the circular linked list of buckets.
 * - `results_` always points to the last bucket
 */
void TileQueryTask::addResult(uint32_t item)
{
	if (results_->isFull())
	{
		QueryResults* next = new QueryResults();
		QueryResults* last = (results_ == QueryResults::EMPTY) ? next : results_;
		next->count = 0;
		next->pTile = pTile_;
		next->next = last->next;
		last->next = next;
		results_ = next;
	}
	results_->items[results_->count++] = item;
}


} // namespace geodesk
