#ifndef _RANGELIST16_H_
#define _RANGELIST16_H_

#include "Types.h"
#include "List.h"
#include "PtrMacro.h"

#define RANGELIST16_HASH(a)		((((a) & m_nMask) << m_nShift) >> 16)

namespace Framework
{
	template <class T, const uint32 m_nMask, const uint8 m_nShift> class CRangeList16
	{
		enum RANGELIST16MAX
		{
			RANGELIST16MAX = 0x10000
		};

	public:
		CRangeList16()
		{
			unsigned int i;
			m_pSubList = new CList<T>*[RANGELIST16MAX];
			for(i = 0; i < RANGELIST16MAX; i++)
			{
				m_pSubList[i] = NULL;
			}
			
		}
		~CRangeList16()
		{
			unsigned int i;
			for(i = 0; i < RANGELIST16MAX; i++)
			{
				DELETEPTR(m_pSubList[i]);
			}
			delete [] m_pSubList;
		}

		void Insert(T* pValue, uint32 nMin, uint32 nMax)
		{
			uint32 nHashMin;
			uint32 nHashMax;

			m_List.Insert(pValue);

			nHashMin = RANGELIST16_HASH(nMin);
			nHashMax = RANGELIST16_HASH(nMax);

			for(i = nHashMin; i <= nHashMax; i++)
			{
				InsertHashed(pValue, i);
			}
		}

		T* Find(uint32 nKey)
		{
			uint32 nHashKey;

			nHashKey = RANGELIST16_HASH(nAddress);


		}

	private:

		void InsertHashed(T* pValue, uint32 nHash)
		{
			if(m_pSubList[nHash] == NULL)
			{
				m_pSubList = new CList<T>;
			}
			m_pSubList[nHash]->Insert(pValue);
		}

		CList<T>		m_List;
		CList<T>**		m_pSubList;
	};
}

#endif