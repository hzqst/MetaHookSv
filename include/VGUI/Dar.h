//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds the enumerated list of default cursors
//
// $NoKeywords: $
//=============================================================================//

#ifndef DAR_H
#define DAR_H

#ifdef _WIN32
#pragma once
#endif

#include <stdlib.h>
#include <string.h>
#include "VGUI.h"
#include <tier1/utlvector.h>

#include <tier0/memdbgon.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Simple lightweight dynamic array implementation
//-----------------------------------------------------------------------------
template<class ELEMTYPE> class Dar : public CUtlVector< ELEMTYPE >
{
	typedef CUtlVector< ELEMTYPE > BaseClass;
	
public:
	Dar()
	{
	}
	Dar(int initialCapacity) :
		BaseClass( 0, initialCapacity )
	{
	}

public:
	void SetCount(int count)
	{
		BaseClass::EnsureCount( count );
	}
	int GetCount()
	{
		return BaseClass::Count();
	}
	int AddElement(ELEMTYPE elem)
	{
		return BaseClass::AddToTail( elem );
	}
	void MoveElementToEnd( ELEMTYPE elem )
	{
		if (BaseClass::Count() == 0 )
			return;

		// quick check to see if it's already at the end
		if (BaseClass::Element(BaseClass::Count() - 1 ) == elem )
			return;

		int idx = Find( elem );
		if ( idx == BaseClass::InvalidIndex() )
			return;

		Remove( idx );
		AddToTail( elem );
	}
	// returns the index of the element in the array, -1 if not found
	int FindElement(ELEMTYPE elem)
	{
		return BaseClass::Find( elem );
	}
	bool HasElement(ELEMTYPE elem)
	{
		if ( FindElement(elem) != BaseClass::InvalidIndex() )
		{
			return true;
		}
		return false;
	}
	int PutElement(ELEMTYPE elem)
	{
		int index = FindElement(elem);
		if (index >= 0)
		{
			return index;
		}
		return AddElement(elem);
	}
	// insert element at index and move all the others down 1
	void InsertElementAt(ELEMTYPE elem,int index)
	{
		BaseClass::InsertBefore( index, elem );
	}
	void SetElementAt(ELEMTYPE elem,int index)
	{
		BaseClass::EnsureCount( index + 1 );
		BaseClass::Element( index ) = elem;
	}
	void RemoveElementAt(int index)
	{
		BaseClass::Remove( index );
	} 

	void RemoveElementsBefore(int index)
	{
		if ( index <= 0 )
			return;
		BaseClass::RemoveMultiple( 0, index - 1 );
	}  

	void RemoveElement(ELEMTYPE elem)
	{
		BaseClass::FindAndRemove( elem );
	}

	void *GetBaseData()
	{
		return BaseClass::Base();
	}

	void CopyFrom(Dar<ELEMTYPE> &dar)
	{
		BaseClass::CopyArray( dar.Base(), dar.Count() );
	}
};

}

#include <tier0/memdbgoff.h>

#endif // DAR_H
