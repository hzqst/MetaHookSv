template <class ELEMTYPE> class OLDDar
{
public:
	OLDDar(void)
	{
		_count = 0;
		_capacity = 0;
		_data = null;
		EnsureCapacity(4);
	}

	OLDDar(int initialCapacity)
	{
		_count = 0;
		_capacity = 0;
		_data = null;
		EnsureCapacity(initialCapacity);
	}

public:
	void EnsureCapacity(int wantedCapacity)
	{
		if (wantedCapacity <= _capacity)
			return;

		int newCapacity = _capacity;

		if (newCapacity == 0)
			newCapacity = 1;

		while (newCapacity < wantedCapacity)
			newCapacity *= 2;

		ELEMTYPE *newData = new ELEMTYPE[newCapacity]; 
		Assert(newData);
		memset(newData, 0, sizeof(ELEMTYPE) * newCapacity);
		_capacity = newCapacity;

		for (int i = 0; i < _count; i++)
			newData[i] = _data[i];

		delete [] _data;
		_data = newData;
	}

	void SetCount(int count)
	{
		if ((count < 0) || (count > _capacity))
			return;

		_count = count;
	}

	int GetCount(void)
	{
		return _count;
	}

	int Find(ELEMTYPE src)
	{
		for (int i = 0; i < _count; ++i)
		{
			if (_data[i] == src)
				return i;
		}

		return -1;
	}

	void MoveElementToEnd(ELEMTYPE elem)
	{
		if (_count == 0)
			return;

		if (_data[_count - 1] == elem)
			return;

		int idx = Find(elem);

		if (idx == -1)
			return;

		RemoveElementAt(idx);
		AddElement(elem);
	}

	void AddElement(ELEMTYPE elem)
	{
		EnsureCapacity(_count + 1);
		_data[_count] = elem;
		_count++;
	}

	bool HasElement(ELEMTYPE elem)
	{
		for (int i = 0; i < _count; i++)
		{
			if (_data[i] == elem)
				return true;
		}

		return false;
	}

	void PutElement(ELEMTYPE elem)
	{
		if (HasElement(elem))
			return;

		AddElement(elem);
	}

	void InsertElementAt(ELEMTYPE elem, int index)
	{
		if ((index < 0) || (index > _count))
			return;

		if ((index == _count) || (_count == 0))
		{
			AddElement(elem);
		}
		else
		{
			AddElement(elem);

			for (int i = _count - 1; i > index; i--)
				_data[i] = _data[i - 1];

			_data[index] = elem;
		}
	}

	void SetElementAt(ELEMTYPE elem,int index)
	{
		if ((index < 0) || (index >= _count))
			return;

		_data[index] = elem;
	}

	void RemoveElementAt(int index)
	{
		if ((index < 0) || (index >= _count))
			return;

		for (int i = index; i < (_count - 1); i++)
			_data[i] = _data[i + 1];

		_count--;
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

	void RemoveAll(void)
	{
		_count = 0;
	}

	ELEMTYPE operator [] (int index)
	{
		if ((index < 0) || (index >= _count))
			return null;

		return _data[index];
	}

protected:
	int _count;
	int _capacity;
	ELEMTYPE *_data;
};