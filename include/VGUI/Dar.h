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

	ELEMTYPE GetRawElementAt(int index)
	{
		return m_pElements[index];
	}
};

template<class ELEMTYPE> class Dar_Legacy
{
public:
	Dar_Legacy()
	{
		_count = 0;
		_capacity = 0;
		_data = null;
	}
	Dar_Legacy(int initialCapacity)
	{
		_count = 0;
		_capacity = 0;
		_data = null;
		EnsureCapacity(initialCapacity);
	}
	~Dar_Legacy()
	{
		delete[] _data;
	}

public:
	void EnsureCapacity(int wantedCapacity)
	{
		if (wantedCapacity <= _capacity) { return; }

		//double capacity until it is >= wantedCapacity
		//this could be done with math, but iterative is just so much more fun
		int newCapacity = _capacity;
		if (newCapacity == 0) { newCapacity = 1; }
		while (newCapacity < wantedCapacity) { newCapacity *= 2; }

		//allocate and zero newData
		ELEMTYPE* newData = new ELEMTYPE[newCapacity];
		if (newData == null) { exit(0); return; }
		memset(newData, 0, sizeof(ELEMTYPE) * newCapacity);
		_capacity = newCapacity;

		//copy data into newData
		for (int i = 0; i < _count; i++) { newData[i] = _data[i]; }

		delete[] _data;
		_data = newData;
	}
	void SetCount(int count)
	{
		if ((count < 0) || (count > _capacity))
		{
			return;
		}
		_count = count;
	}
	int GetCount()
	{
		return _count;
	}
	int AddElement(ELEMTYPE elem)
	{
		EnsureCapacity(_count + 1);
		_data[_count] = elem;
		_count++;
		return _count - 1;
	}
	void MoveElementToEnd(ELEMTYPE elem)
	{
		// quick check to see if it's already at the end
		if (_data[_count - 1] == elem)
			return;

		// search backwards in the array looking for the element (since it's probably close to the end already)
		for (int i = _count - 2; i >= 0; i--)
		{
			if (_data[i] == elem)
			{
				// slide the data back
				Q_memmove(_data + i, _data + i + 1, sizeof(ELEMTYPE) * (_count - (i + 1)));
				_data[_count - 1] = elem;
				return;
			}
		}
	}
	// returns the index of the element in the array, -1 if not found
	int FindElement(ELEMTYPE elem)
	{
		for (int i = 0; i < _count; i++)
		{
			if (_data[i] == elem)
			{
				return i;
			}
		}
		return -1;
	}
	bool HasElement(ELEMTYPE elem)
	{
		if (FindElement(elem) >= 0)
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
	void InsertElementAt(ELEMTYPE elem, int index)
	{
		if ((index < 0) || (index > _count))
		{
			return;
		}
		if ((index == _count) || (_count == 0))
		{
			AddElement(elem);
		}
		else
		{
			AddElement(elem); //just to make sure it is big enough
			for (int i = _count - 1; i > index; i--)
			{
				_data[i] = _data[i - 1];
			}
			_data[index] = elem;
		}
	}
	void SetElementAt(ELEMTYPE elem, int index)
	{
		if ((index < 0) || (index >= _capacity))
		{
			return;
		}

		if (index >= _count)
		{
			// null out from index to _count
			for (int i = _count; i < index; i++)
			{
				_data[i] = null;
			}
			SetCount(index + 1);
		}

		_data[index] = elem;
	}
	void RemoveElementAt(int index)
	{
		if ((index < 0) || (index >= _count))
		{
			return;
		}

		//slide everything to the right of index, left one.
		for (int i = index; i < (_count - 1); i++)
		{
			_data[i] = _data[i + 1];
		}
		_count--;
	}

	void RemoveElementsBefore(int index)
	{
		if ((index < 0) || (index >= _count))
		{
			return;
		}

		//slide everything to the right of index, left to the start.
		for (int i = index; i < (_count); i++)
		{
			_data[i - index] = _data[i];
		}
		_count = _count - index;
	}

	void RemoveElement(ELEMTYPE elem)
	{
		for (int i = 0; i < _count; i++)
		{
			if (_data[i] == elem)
			{
				RemoveElementAt(i);
				break;
			}
		}
	}
	void RemoveAll()
	{
		_count = 0;
	}
	ELEMTYPE operator[](int index)
	{
		if ((index < 0) || (index >= _count))
		{
			return null;
		}
		return _data[index];
	}

	void* GetBaseData()
	{
		return _data;
	}

	void CopyFrom(Dar<ELEMTYPE>& dar)
	{
		EnsureCapacity(dar.GetCount());
		memcpy(_data, dar._data, sizeof(ELEMTYPE) * dar.GetCount());
		_count = dar._count;
	}

protected:
	int       _count;
	int       _capacity;
	ELEMTYPE* _data;
};

}

#include <tier0/memdbgoff.h>

#endif // DAR_H
