#pragma once

template <typename StructType>
struct COsStructQueue
{
public:
	typedef COsStructManager<StructType> StructManager;

	class iterator
	{
	public:
		iterator(const StructManager& container, uint32* idPtr) : m_container(container), m_idPtr(idPtr) {}

		iterator& operator ++()
		{
			MoveNext();
			return (*this);
		}

		iterator operator ++(int)
		{
			iterator copy(*this);
			MoveNext();
			return copy;
		}

		bool operator !=(const iterator& rhs) const
		{
			return m_idPtr != rhs.m_idPtr;
		}

		std::pair<uint32, StructType*> operator *() const
		{
			assert(m_idPtr);
			return std::make_pair(*m_idPtr, m_container[*m_idPtr]);
		}

	private:
		void MoveNext()
		{
			assert(m_idPtr);
			auto nextItem = m_container[*m_idPtr];
			m_idPtr = &nextItem->nextId;
			if(*m_idPtr == 0)
			{
				m_idPtr = nullptr;
			}
		}

		const StructManager& m_container;
		uint32* m_idPtr = nullptr;
	};

	COsStructQueue(StructManager& items, uint32* headIdPtr)
		: m_items(items)
		, m_headIdPtr(headIdPtr)
	{

	}

	COsStructQueue(const COsStructQueue&) = delete;
	COsStructQueue& operator =(const COsStructQueue) = delete;

	bool IsEmpty() const
	{
		return (*m_headIdPtr == 0);
	}

	void PushFront(uint32 id)
	{
		uint32 nextId = (*m_headIdPtr);
		(*m_headIdPtr) = id;
		auto item = m_items[id];
		item->nextId = nextId;
	}

	void PushBack(uint32 id)
	{
		auto nextId = m_headIdPtr;
		while(1)
		{
			if(*nextId == 0) break;
			auto nextItem = m_items[*nextId];
			nextId = &nextItem->nextId;
		}
		*nextId = id;
	}

	void AddBefore(uint32 beforeId, uint32 id)
	{
		auto newItem = m_items[id];
		auto nextId = m_headIdPtr;
		while(1)
		{
			if(*nextId == 0)
			{
				//Id not found
				assert(false);
				break;
			}
			auto nextItem = m_items[*nextId];
			if((*nextId) == beforeId)
			{
				(*nextId) = id;
				newItem->nextId = beforeId;
				break;
			}
			nextId = &nextItem->nextId;
		}
	}

	void Unlink(uint32 id)
	{
		auto nextId = m_headIdPtr;
		while(1)
		{
			if(*nextId == 0)
			{
				//Id not found
				assert(false);
				break;
			}
			auto nextItem = m_items[*nextId];
			if((*nextId) == id)
			{
				(*nextId) = nextItem->nextId;
				nextItem->nextId = 0;
				break;
			}
			nextId = &nextItem->nextId;
		}
	}

	iterator begin() const
	{
		return iterator(m_items, IsEmpty() ? nullptr : m_headIdPtr);
	}

	iterator end() const
	{
		return iterator(m_items, nullptr);
	}

private:
	uint32*			m_headIdPtr = nullptr;
	StructManager&	m_items;
};
