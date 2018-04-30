#pragma once
#include <cassert>

template <typename StructType>
class COsStructManager
{
public:
	class iterator
	{
	public:
		iterator(const COsStructManager& container, uint32 id)
		    : m_container(container)
		    , m_id(id)
		{
		}

		iterator& operator++()
		{
			m_id++;
			return (*this);
		}

		iterator operator++(int)
		{
			iterator copy(*this);
			m_id++;
			return copy;
		}

		bool operator!=(const iterator& rhs) const
		{
			return m_id != rhs.m_id;
		}

		StructType* operator*() const
		{
			return m_container[m_id];
		}

		operator uint32() const
		{
			return m_id;
		}

	private:
		const COsStructManager& m_container;
		uint32 m_id = 0;
	};

	enum
	{
		INVALID_ID = -1
	};

	COsStructManager(StructType* structBase, uint32 idBase, uint32 structMax)
	    : m_structBase(structBase)
	    , m_structMax(structMax)
	    , m_idBase(idBase)
	{
		assert(m_idBase != 0);
	}

	COsStructManager(const COsStructManager&) = delete;
	COsStructManager& operator=(const COsStructManager) = delete;

	StructType* GetBase() const
	{
		return m_structBase;
	}

	StructType* operator[](uint32 index) const
	{
		index -= m_idBase;
		if(index >= m_structMax)
		{
			return nullptr;
		}
		auto structPtr = m_structBase + index;
		if(!structPtr->isValid)
		{
			return nullptr;
		}
		return structPtr;
	}

	uint32 Allocate() const
	{
		for(unsigned int i = 0; i < m_structMax; i++)
		{
			if(!m_structBase[i].isValid)
			{
				m_structBase[i].isValid = true;
				return (i + m_idBase);
			}
		}
		return INVALID_ID;
	}

	void Free(uint32 id)
	{
		StructType* structPtr = (*this)[id];
		if(!structPtr->isValid)
		{
			throw std::exception();
		}
		structPtr->isValid = false;
	}

	void FreeAll()
	{
		for(unsigned int i = 0; i < m_structMax; i++)
		{
			m_structBase[i].isValid = false;
		}
	}

	iterator begin() const
	{
		return iterator(*this, m_idBase);
	}

	iterator end() const
	{
		return iterator(*this, m_idBase + m_structMax);
	}

private:
	StructType* m_structBase = nullptr;
	uint32 m_structMax = 0;
	uint32 m_idBase = 0;
};
