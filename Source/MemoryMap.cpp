#include <assert.h>
#include <stdio.h>
#include "MemoryMap.h"

CMemoryMap::~CMemoryMap()
{
	DeleteMap(m_readMap);
	DeleteMap(m_writeMap);
    DeleteMap(m_instructionMap);
}

void CMemoryMap::InsertReadMap(uint32 nStart, uint32 nEnd, void* pPointer, unsigned char nKey)
{
	InsertMap(m_readMap, nStart, nEnd, pPointer, nKey);
}

void CMemoryMap::InsertReadMap(uint32 start, uint32 end, const MemoryMapHandlerType& handler, unsigned char key)
{
    InsertMap(m_readMap, start, end, handler, key);
}

void CMemoryMap::InsertWriteMap(uint32 nStart, uint32 nEnd, void* pPointer, unsigned char nKey)
{
	InsertMap(m_writeMap, nStart, nEnd, pPointer, nKey);
}

void CMemoryMap::InsertWriteMap(uint32 start, uint32 end, const MemoryMapHandlerType& handler, unsigned char key)
{
    InsertMap(m_writeMap, start, end, handler, key);
}

void CMemoryMap::InsertInstructionMap(uint32 start, uint32 end, void* pointer, unsigned char key)
{
    InsertMap(m_instructionMap, start, end, pointer, key);
}

CMemoryMap::MEMORYMAPELEMENT* CMemoryMap::GetReadMap(uint32 address)
{
    return GetMap(m_readMap, address);
}

CMemoryMap::MEMORYMAPELEMENT* CMemoryMap::GetWriteMap(uint32 address)
{
    return GetMap(m_writeMap, address);
}

void CMemoryMap::SetWriteNotifyHandler(const WriteNotifyHandlerType& WriteNotifyHandler)
{
	m_WriteNotifyHandler = WriteNotifyHandler;
}

void CMemoryMap::InsertMap(MemoryMapListType& memoryMap, uint32 start, uint32 end, void* pointer, unsigned char key)
{
    MEMORYMAPELEMENT element;
	element.nStart      = start;
	element.nEnd        = end;
	element.pPointer    = pointer;
	element.nType       = MEMORYMAP_TYPE_MEMORY;
    memoryMap[key] = element;
}

void CMemoryMap::InsertMap(MemoryMapListType& memoryMap, uint32 start, uint32 end, const MemoryMapHandlerType& handler, unsigned char key)
{
    MEMORYMAPELEMENT element;
	element.nStart      = start;
	element.nEnd        = end;
    element.handler     = handler;
    element.pPointer    = NULL;
	element.nType       = MEMORYMAP_TYPE_FUNCTION;
    memoryMap[key] = element;
}

void CMemoryMap::DeleteMap(MemoryMapListType& memoryMap)
{
    memoryMap.clear();
}

CMemoryMap::MEMORYMAPELEMENT* CMemoryMap::GetMap(MemoryMapListType& memoryMap, uint32 nAddress)
{
    for(MemoryMapListType::iterator element(memoryMap.begin());
        memoryMap.end() != element; element++)
    {
        MEMORYMAPELEMENT& mapElement(element->second);
		if(nAddress <= mapElement.nEnd)
		{
			if(!(nAddress >= mapElement.nStart)) return NULL;
			return &mapElement;
		}
    }
    return NULL;
}

uint8 CMemoryMap::GetByte(uint32 nAddress)
{
	MEMORYMAPELEMENT* e;
	e = GetMap(m_readMap, nAddress);
	if(e == NULL) return 0xCC;
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		return *(uint8*)&((uint8*)e->pPointer)[nAddress - e->nStart];
		break;
	case MEMORYMAP_TYPE_FUNCTION:
        return static_cast<uint8>(e->handler(nAddress, 0));
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
	e = GetMap(m_writeMap, nAddress);
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
        e->handler(nAddress, nValue);
		break;
	default:
		assert(0);
		break;
	}

	if(m_WriteNotifyHandler)
	{
		m_WriteNotifyHandler(nAddress);
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
	e = GetMap(m_readMap, nAddress);
	if(e == NULL) return 0xCCCC;
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		return *(uint16*)&((uint8*)e->pPointer)[nAddress - e->nStart];
		break;
	default:
        return static_cast<uint16>(e->handler(nAddress, 0));
		break;
	}
}

uint32 CMemoryMap_LSBF::GetWord(uint32 nAddress)
{
    assert((nAddress & 0x03) == 0);
	MEMORYMAPELEMENT* e = GetMap(m_readMap, nAddress);
	if(e == NULL) return 0xCCCCCCCC;
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		return *(uint32*)&((uint8*)e->pPointer)[nAddress - e->nStart];
		break;
	case MEMORYMAP_TYPE_FUNCTION:
        return e->handler(nAddress, 0);
		break;
	default:
		assert(0);
		return 0xCCCCCCCC;
		break;
	}
}

uint32 CMemoryMap_LSBF::GetInstruction(uint32 address)
{
    assert((address & 0x03) == 0);
	MEMORYMAPELEMENT* e = GetMap(m_instructionMap, address);
	if(e == NULL) return 0xCCCCCCCC;
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		return *reinterpret_cast<uint32*>(&reinterpret_cast<uint8*>(e->pPointer)[address - e->nStart]);
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
	e = GetMap(m_writeMap, nAddress);
	if(e == NULL) 
	{
		printf("MemoryMap: Wrote to unmapped memory (0x%0.8X, 0x%0.4X).\r\n", nAddress, nValue);
		return;
	}
	switch(e->nType)
	{
	case MEMORYMAP_TYPE_MEMORY:
		*reinterpret_cast<uint16*>(&reinterpret_cast<uint8*>(e->pPointer)[nAddress - e->nStart]) = nValue;
		break;
	case MEMORYMAP_TYPE_FUNCTION:
		e->handler(nAddress, nValue);
		break;
	default:
		assert(0);
		break;
	}

	if(m_WriteNotifyHandler)
	{
		m_WriteNotifyHandler(nAddress);
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
	e = GetMap(m_writeMap, nAddress);
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
        e->handler(nAddress, nValue);
		break;
	default:
		assert(0);
		break;
	}
	
	if(m_WriteNotifyHandler)
	{
		m_WriteNotifyHandler(nAddress);
	}
}
