#ifndef _LOCKFREEQUEUE_H_
#define _LOCKFREEQUEUE_H_

template <typename ValueType>
class CLockFreeQueue
{
public:
	CLockFreeQueue(unsigned int maxItemCount)
		: m_prodIndex(0)
		, m_consIndex(0)
		, m_itemCount(0)
		, m_maxItemCount(maxItemCount)
	{
		m_items = new ValueType[m_maxItemCount];
	}

	virtual ~CLockFreeQueue()
	{
		delete [] m_items;
	}

	bool TryPop(ValueType& item)
	{
		if(m_itemCount == 0)
		{
			return false;
		}

		item = m_items[m_consIndex];

		m_consIndex++;
		m_consIndex %= m_maxItemCount;

		InterlockedDecrement(&m_itemCount);

		return true;
	}

	bool TryPush(const ValueType& item)
	{
		if(m_itemCount == m_maxItemCount)
		{
			return false;
		}

		m_items[m_prodIndex] = item;

		m_prodIndex++;
		m_prodIndex %= m_maxItemCount;

		InterlockedIncrement(&m_itemCount);

		return true;
	}

private:
	ValueType*			m_items;
	LONG				m_itemCount;
	unsigned int		m_maxItemCount;
	unsigned int		m_prodIndex;
	unsigned int		m_consIndex;
};

#endif
