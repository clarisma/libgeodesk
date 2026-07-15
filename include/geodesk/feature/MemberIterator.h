// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <geodesk/feature/RelatedIterator.h>
#include <clarisma/util/ShortVarString.h>
#include <geodesk/feature/FeaturePtr.h>
#include <geodesk/feature/FeatureStore.h>
#include <geodesk/filter/RoleFilter.h>

#include "geodesk/filter/ComboFilter.h"

namespace geodesk {

// TODO: need to bump the refcount of `store` to ensure that store does not 
// get destroyed if the MemberIterator is used as a standalone object?

// Warning: MemberIterator cannot be used on empty relations, need to check first

// TODO: For the sake of the API, we should treat local and global strings the same

// TODO: Warning, MemberIterator is not threadsafe because it uses a Python string
// to track the role
// 4/3/24: The Python role string object is now created lazily, which means that
//  MemberIterator is now threadsafe as long as the Python role string is not 
//  accessed (via borrowCurrentRole())

/// \cond lowlevel
///

class MemberIteratorBase : public RelatedIterator<MemberIteratorBase,FeaturePtr,1,2>
{
public:
	MemberIteratorBase(FeatureStore* store, DataPtr pMembers,
		FeatureTypes types, const MatcherHolder* matcher, const Filter* filter,
		const RoleFilter* roleFilter = nullptr) :
		RelatedIterator(store, pMembers, Tex::MEMBERS_START_TEX,
		matcher, filter),
		types_(types),
		currentRoleCode_(0),
		currentRoleStr_(nullptr),
		roleFilter_(roleFilter)
	{
		#ifdef GEODESK_PYTHON
		// currentRoleObject_ = store->strings().getStringObject(0);
		// TODO: this bumps the refcount; let's use a "borrow" function instead!
		// TODO: check for refcount handling in code below
		// No, see comments below -- must refcount because local strings are
		// treated as owned
		// assert(currentRoleObject_);
		currentRoleObject_ = nullptr;
		#endif
	}

	MemberIteratorBase(FeatureStore* store, DataPtr pMembers) :
		MemberIteratorBase(store, pMembers, FeatureTypes::ALL,
		store->borrowAllMatcher(), nullptr) {}

	~MemberIteratorBase()
	{
		#ifdef GEODESK_PYTHON
		Py_XDECREF(currentRoleObject_);
		#endif
	}

	std::string_view currentRole() const
	{
		if (currentRoleCode_ >= 0)
		{
			return store_->strings().getGlobalString(currentRoleCode_)->toStringView();
		}
		return currentRoleStr_->toStringView();
	}

	StringValue currentRoleStr() const
	{
		if (currentRoleCode_ >= 0)
		{
			return store_->strings().getGlobalString(currentRoleCode_);
		}
		return currentRoleStr_;
	}

	int currentRoleCode() const
	{
		return currentRoleCode_;
	}

	bool hasLocalRole() const
	{
		return currentRoleCode_ < 0;
	}

	#ifdef GEODESK_PYTHON
	/**
	 * Obtains a borrowed reference to the Python string object that
	 * represents the role of the current member.
	 */
	PyObject* borrowCurrentRole() // const
	{
		if (!currentRoleObject_)
		{
			if (currentRoleCode_ >= 0)
			{
				currentRoleObject_ = store_->strings().getStringObject(currentRoleCode_);
				assert(currentRoleObject_);
			}
			else
			{
				assert(currentRoleStr_);
				currentRoleObject_ = Python::toStringObject(*currentRoleStr_);

				//currentRoleObject_ = Python::toStringObject(
				//	currentRoleStr_->data(), currentRoleStr_->length());

				// TODO: It is possible that string creation fails if the
				//  UTF-8 encoding is bad; a GOL should never allow
				//  invalid UTF-8 data to be stored
				/*
				if(!currentRoleObject_)
				{
					printf("Bad string: %s\n  Ptr = %p\n\n",
						currentRoleStr_->toString().c_str(),
						currentRoleStr_->data());
					currentRoleObject_ = store_->strings().getStringObject(0);
					PyErr_Print();
					PyErr_Clear();
					// TODO: for debug only !!!!
				}
				*/
				assert(currentRoleObject_);
			}
			assert(currentRoleObject_);
		}
		return currentRoleObject_;
	}
	#endif

	bool readAndAcceptRole()    // CRTP override
	{
		if (member_ & MemberFlags::DIFFERENT_ROLE)
		{
			int rawRole = p_.getUnsignedShort();
			p_ += 2;
			if (rawRole & 1)	[[likely]]
			{
				// common role
				currentRoleCode_ = rawRole >> 1;
				// TODO: ensure this will be unsigned
				currentRoleStr_ = nullptr;
#ifdef GEODESK_PYTHON
				if (currentRoleObject_)
				{
					Py_DECREF(currentRoleObject_);
					currentRoleObject_ = nullptr;
				}
				// move out of if?
				// TODO: This is wrong, needs to borrow!
				//  No, it has to addref, because refs to local strings are owned
				//  and we have no way to distinguish between them that is cheaper
				//  than refcounting
				// currentRoleObject_ = store_->strings().getStringObject(currentRoleCode_);
				// assert(currentRoleObject_);
#endif
			}
			else
			{
				rawRole |= static_cast<int>(p_.getShort()) << 16;
				currentRoleCode_ = -1;
				currentRoleStr_ = reinterpret_cast<const clarisma::ShortVarString*>(
					p_.ptr() + ((rawRole >> 1) - 2)); // signed
#ifdef GEODESK_PYTHON
				if (currentRoleObject_)
				{
					Py_DECREF(currentRoleObject_);
					currentRoleObject_ = nullptr;
				}
#endif
				p_ += 2;
			}
			if (roleFilter_ && !roleFilter_->acceptRole(currentRoleCode_, currentRoleStr_))
			{
				currentRoleCode_ = REJECTED_ROLE;
			}
		}
		return currentRoleCode_ != REJECTED_ROLE;
	}

	bool accept(FeaturePtr feature) const	// CRTP override
	{
		if (!types_.acceptFlags(feature.flags())) return false;
		if (!matcher_->mainMatcher().accept(feature)) return false;
        return filter_ == nullptr || filter_->accept(
			store_, feature, FastFilterHint());
	}

protected:
	static constexpr int REJECTED_ROLE = -2;

	FeatureTypes types_;
	int currentRoleCode_;
	const clarisma::ShortVarString* currentRoleStr_;
	const RoleFilter* roleFilter_;
	#ifdef GEODESK_PYTHON
	PyObject* currentRoleObject_;
	#endif
};

class MemberIterator : public MemberIteratorBase
{
public:
	MemberIterator(FeatureStore* store, DataPtr pMembers,
		FeatureTypes types, const MatcherHolder* matcher, const Filter* filter) :
		MemberIteratorBase(store, pMembers, types, matcher, filter)
	{
		if (filter)	[[unlikely]]
		{
			if (filter->isRoleFilter())
			{
				roleFilter_ = reinterpret_cast<const RoleFilter*>(filter);
				filter_ = nullptr;
			}
			else if (filter->isCombo())
			{
				// extract RoleFilter from combo
				const ComboFilter* comboFilter = reinterpret_cast<const ComboFilter*>(filter);
				roleFilter_ = comboFilter->roleFilter();
				if (roleFilter_)
				{
					ownedFilter_ = comboFilter->withoutRoleFilter();
					filter_ = ownedFilter_;
				}
			}
		}
	}

	~MemberIterator()
	{
		if (ownedFilter_)	[[unlikely]]
		{
			ownedFilter_->release();
		}
	}

private:
	const Filter* ownedFilter_ = nullptr;
};


/// \endcond
} // namespace geodesk
