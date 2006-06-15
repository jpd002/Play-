#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include "./Framework/List.h"

namespace Framework
{

	template <class Type> class CHashTable
	{
	public:
		CHashTable()
		{
			m_pSubList = new CList<Type>[0x100];
		}
		~CHashTable()
		{
			delete [] m_pSubList;
		}

		unsigned int Insert(Type* pData, uint32 nKey)
		{
			m_List.Insert(pData, nKey);
		}

	private:
		uint8 Hash(uint32 nKey)
		{
			uint8* pPtr;
			pPtr = (uint8*)&nKey;
			return pPtr[0] ^ pPtr[1] ^ pPtr[2] ^ pPtr[3];
		}

		CList<Type>		m_List;
		CList<Type>*	m_pSubList;
	};

}


#endif