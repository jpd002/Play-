#include "MemoryViewPtr.h"

CMemoryViewPtr::CMemoryViewPtr(HWND hParent, const RECT& rect)
: CMemoryView(hParent, rect)
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
