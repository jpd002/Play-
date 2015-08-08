#pragma once

template <typename StructType>
struct COsStructQueue
{
public:
	typedef COsStructManager<StructType> StructManager;

	COsStructQueue(StructManager& items, uint32* headIdPtr)
		: m_items(items)
		, m_headIdPtr(headIdPtr)
	{

	}

	COsStructQueue(const COsStructQueue&) = delete;
	COsStructQueue& operator =(const COsStructQueue) = delete;

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

private:
	uint32*			m_headIdPtr = nullptr;
	StructManager&	m_items;
};
