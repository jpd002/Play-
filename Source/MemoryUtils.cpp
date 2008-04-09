#include "MemoryUtils.h"

uint32 CMemoryUtils::GetByteProxy(CMIPS* pCtx, uint32 nAddress)
{
	return (uint32)pCtx->m_pMemoryMap->GetByte(nAddress);
}

uint32 CMemoryUtils::GetHalfProxy(CMIPS* pCtx, uint32 nAddress)
{
	return (uint32)pCtx->m_pMemoryMap->GetHalf(nAddress);
}

uint32 CMemoryUtils::GetWordProxy(CMIPS* pCtx, uint32 nAddress)
{
	return pCtx->m_pMemoryMap->GetWord(nAddress);
}

void CMemoryUtils::SetByteProxy(CMIPS* pCtx, uint32 nValue, uint32 nAddress)
{
	pCtx->m_pMemoryMap->SetByte(nAddress, (uint8)(nValue & 0xFF));
}

void CMemoryUtils::SetHalfProxy(CMIPS* pCtx, uint32 nValue, uint32 nAddress)
{
	pCtx->m_pMemoryMap->SetHalf(nAddress, (uint16)(nValue & 0xFFFF));
}

void CMemoryUtils::SetWordProxy(CMIPS* pCtx, uint32 nValue, uint32 nAddress)
{
	pCtx->m_pMemoryMap->SetWord(nAddress, nValue);
}
