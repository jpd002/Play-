#include <assert.h>
#include <stdio.h>
#include "MemoryMap.h"

using namespace Framework;

CMemoryMap::~CMemoryMap()
{
	DeleteMap(&m_Read);
	DeleteMap(&m_Write);
}

void CMemoryMap::InsertReadMap(uint32 nStart, uint32 nEnd, void* pPointer, MEMORYMAP_TYPE nType, unsigned char nKey)
{
	InsertMap(&m_Read, nStart, nEnd, pPointer, nType, nKey);
}

void CMemoryMap::InsertWriteMap(uint32 nStart, uint32 nEnd, void* pPointer, MEMORYMAP_TYPE nType, unsigned char nKey)
{
	InsertMap(&m_Write, nStart, nEnd, pPointer, nType, nKey);
}

void CMemoryMap::InsertMap(CList<MEMORYMAPELEMENT>* pMap, uint32 nStart, uint32 nEnd, void* pPointer, MEMORYMAP_TYPE nType, unsigned char nKey)
{
	MEMORYMAPELEMENT* e;
	e = (MEMORYMAPELEMENT*)malloc(sizeof(MEMORYMAPELEMENT));
	e->nStart		= nStart;
	e->nEnd			= nEnd;
	e->pPointer		= pPointer;
	e->nType		= nType;
	pMap->Insert(e, nKey);
}

void CMemoryMap::DeleteMap(CList<MEMORYMAPELEMENT>* pMap)
{
	while(pMap->Count())
	{
		free(pMap->Pull());
	}
}

MEMORYMAPELEMENT* CMemoryMap::GetMap(CList<MEMORYMAPELEMENT>* pMap, uint32 nAddress)
{
	MEMORYMAPELEMENT* e;
	CList<MEMORYMAPELEMENT>::ITERATOR It;
	It = pMap->Begin();
	e = (*It);
	while(e != NULL)
	{
		if(nAddress <= e->nEnd)
		{
			if(!(nAddress >= e->nStart)) return NULL;
			return e;
		}
		It++;
		e = (*It);
	}
	return e;
}

uint8 CMemoryMap::GetByte(uint32 nAddress)
{
	MEMORYMAPELEMENT* e;
	e = GetMap(&m_Read, nAddress);
	if(e == NULL) return 0xCC;
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		return *(uint8*)&((uint8*)e->pPointer)[nAddress - e->nStart];
		break;
	default:
		assert(0);
		return 0xCC;
		break;
	}
}

void CMemoryMap::SetByte(uint32 nAddress, uint8 nValue)
{
	MEMORYMAPELEMENT* e;
	e = GetMap(&m_Write, nAddress);
	if(e == NULL)
	{
		printf("MemoryMap: Wrote to unmapped memory (0x%0.8X, 0x%0.4X).\r\n", nAddress, nValue);
		return;
	}
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		*(uint8*)&((uint8*)e->pPointer)[nAddress - e->nStart] = nValue;
		break;
	case MEMORYMAP_TYPE_FUNCTION:
		((void (*)(uint32, uint32))e->pPointer)(nAddress, nValue);
		break;
	default:
		assert(0);
		break;
	}
}

//////////////////////////////////////////////////////////////////
//LSB First Memory Map Implementation
//////////////////////////////////////////////////////////////////

uint16 CMemoryMap_LSBF::GetHalf(uint32 nAddress)
{
	MEMORYMAPELEMENT* e;
	if(nAddress & 0x01)
	{
		//Unaligned access (shouldn't happen)
		assert(0);
	}
	e = GetMap(&m_Read, nAddress);
	if(e == NULL) return 0xCCCC;
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		return *(uint16*)&((uint8*)e->pPointer)[nAddress - e->nStart];
		break;
	default:
		assert(0);
		return 0xCCCC;
		break;
	}
}

uint32 CMemoryMap_LSBF::GetWord(uint32 nAddress)
{
	MEMORYMAPELEMENT* e;
	if(nAddress & 0x03)
	{
		//Unaligned access (shouldn't happen)
		assert(0);
	}
	e = GetMap(&m_Read, nAddress);
	if(e == NULL) return 0xCCCCCCCC;
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		return *(uint32*)&((uint8*)e->pPointer)[nAddress - e->nStart];
		break;
	case MEMORYMAP_TYPE_FUNCTION:
		return ((uint32 (*)(uint32))e->pPointer)(nAddress);
		break;
	default:
		assert(0);
		return 0xCCCCCCCC;
		break;
	}
}

void CMemoryMap_LSBF::SetHalf(uint32 nAddress, uint16 nValue)
{
	MEMORYMAPELEMENT* e;
	if(nAddress & 0x01)
	{
        //Unaligned access (shouldn't happen)
		assert(0);
	}
	e = GetMap(&m_Write, nAddress);
	if(e == NULL) 
	{
		printf("MemoryMap: Wrote to unmapped memory (0x%0.8X, 0x%0.4X).\r\n", nAddress, nValue);
		return;
	}
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		*(uint16*)&((uint8*)e->pPointer)[nAddress - e->nStart] = nValue;
		break;
	default:
		assert(0);
		break;
	}
}

void CMemoryMap_LSBF::SetWord(uint32 nAddress, uint32 nValue)
{
	MEMORYMAPELEMENT* e;
	if(nAddress & 0x03)
	{
        //Unaligned access (shouldn't happen)
		assert(0);
	}
	e = GetMap(&m_Write, nAddress);
	if(e == NULL) 
	{
		printf("MemoryMap: Wrote to unmapped memory (0x%0.8X, 0x%0.8X).\r\n", nAddress, nValue);
		return;
	}
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		*(uint32*)&((uint8*)e->pPointer)[nAddress - e->nStart] = nValue;
		break;
	case MEMORYMAP_TYPE_FUNCTION:
		((void (*)(uint32, uint32))e->pPointer)(nAddress, nValue);
		break;
	default:
		assert(0);
		break;
	}
}
