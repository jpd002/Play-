#include "MemoryUtils.h"
#include "Integer64.h"

uint32 MemoryUtils_GetByteProxy(CMIPS* pCtx, uint32 nAddress)
{
	return (uint32)pCtx->m_pMemoryMap->GetByte(nAddress);
}

uint32 MemoryUtils_GetHalfProxy(CMIPS* pCtx, uint32 nAddress)
{
	return (uint32)pCtx->m_pMemoryMap->GetHalf(nAddress);
}

uint32 MemoryUtils_GetWordProxy(CMIPS* pCtx, uint32 nAddress)
{
	return pCtx->m_pMemoryMap->GetWord(nAddress);
}

uint64 MemoryUtils_GetDoubleProxy(CMIPS* context, uint32 address)
{
	assert((address & 0x07) == 0);
	const CMemoryMap::MEMORYMAPELEMENT* e = context->m_pMemoryMap->GetReadMap(address);
	INTEGER64 result;
#ifdef _DEBUG
	result.q = 0xCCCCCCCCCCCCCCCCull;
#endif
	if(e != NULL)
	{
		switch(e->nType)
		{
		case CMemoryMap::MEMORYMAP_TYPE_MEMORY:
			result.q = *reinterpret_cast<uint64*>(reinterpret_cast<uint8*>(e->pPointer) + (address - e->nStart));
			break;
		case CMemoryMap::MEMORYMAP_TYPE_FUNCTION:
			for(unsigned int i = 0; i < 2; i++)
			{
				result.d[i] = e->handler(address + (i * 4), 0);
			}
			break;
		default:
			assert(0);
			break;
		}
	}
	return result.q;
}

uint128 MemoryUtils_GetQuadProxy(CMIPS* context, uint32 address)
{
	address &= ~0x0F;
	const CMemoryMap::MEMORYMAPELEMENT* e = context->m_pMemoryMap->GetReadMap(address);
	uint128 result;
#ifdef _DEBUG
	memset(&result, 0xCC, sizeof(result));
#endif
	if(e != NULL)
	{
		switch(e->nType)
		{
		case CMemoryMap::MEMORYMAP_TYPE_MEMORY:
			result = *reinterpret_cast<uint128*>(reinterpret_cast<uint8*>(e->pPointer) + (address - e->nStart));
			break;
		case CMemoryMap::MEMORYMAP_TYPE_FUNCTION:
			for(unsigned int i = 0; i < 4; i++)
			{
				result.nV[i] = e->handler(address + (i * 4), 0);
			}
			break;
		default:
			assert(0);
			break;
		}
	}
	return result;
}

void MemoryUtils_SetByteProxy(CMIPS* pCtx, uint32 nValue, uint32 nAddress)
{
	pCtx->m_pMemoryMap->SetByte(nAddress, (uint8)(nValue & 0xFF));
}

void MemoryUtils_SetHalfProxy(CMIPS* pCtx, uint32 nValue, uint32 nAddress)
{
	pCtx->m_pMemoryMap->SetHalf(nAddress, (uint16)(nValue & 0xFFFF));
}

void MemoryUtils_SetWordProxy(CMIPS* pCtx, uint32 nValue, uint32 nAddress)
{
	pCtx->m_pMemoryMap->SetWord(nAddress, nValue);
}

void MemoryUtils_SetDoubleProxy(CMIPS* context, uint64 value64, uint32 address)
{
	assert((address & 0x07) == 0);
	INTEGER64 value;
	value.q = value64;
	const CMemoryMap::MEMORYMAPELEMENT* e = context->m_pMemoryMap->GetWriteMap(address);
	if(e == NULL) 
	{
		printf("MemoryMap: Wrote to unmapped memory (0x%0.8X, [0x%0.8X, 0x%0.8X]).\r\n", 
			address, value.d0, value.d1);
		return;
	}
	switch(e->nType)
	{
	case CMemoryMap::MEMORYMAP_TYPE_MEMORY:
		*reinterpret_cast<uint64*>(reinterpret_cast<uint8*>(e->pPointer) + (address - e->nStart)) = value.q;
		break;
	case CMemoryMap::MEMORYMAP_TYPE_FUNCTION:
		for(unsigned int i = 0; i < 2; i++)
		{
			e->handler(address + (i * 4), value.d[i]);
		}
		break;
	default:
		assert(0);
		break;
	}
}

void MemoryUtils_SetQuadProxy(CMIPS* context, const uint128& value, uint32 address)
{
	address &= ~0x0F;
	const CMemoryMap::MEMORYMAPELEMENT* e = context->m_pMemoryMap->GetWriteMap(address);
	if(e == NULL) 
	{
		printf("MemoryMap: Wrote to unmapped memory (0x%0.8X, [0x%0.8X, 0x%0.8X, 0x%0.8X, 0x%0.8X]).\r\n", 
			address, value.nV0, value.nV1, value.nV2, value.nV3);
		return;
	}
	switch(e->nType)
	{
	case CMemoryMap::MEMORYMAP_TYPE_MEMORY:
		*reinterpret_cast<uint128*>(reinterpret_cast<uint8*>(e->pPointer) + (address - e->nStart)) = value;
		break;
	case CMemoryMap::MEMORYMAP_TYPE_FUNCTION:
		for(unsigned int i = 0; i < 4; i++)
		{
			e->handler(address + (i * 4), value.nV[i]);
		}
		break;
	default:
		assert(0);
		break;
	}
}
