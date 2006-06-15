#include "MemoryViewPtr.h"

CMemoryViewPtr::CMemoryViewPtr(HWND hParent, RECT* pR) :
CMemoryView(hParent, pR)
{
	m_pData = NULL;
}

void CMemoryViewPtr::SetData(void* pData, uint32 nSize)
{
	m_pData = pData;
	SetMemorySize(nSize);
}

uint8 CMemoryViewPtr::GetByte(uint32 nAddress)
{
	return ((uint8*)m_pData)[nAddress];
}
