#ifndef _MEMORYVIEWPTR_H_
#define _MEMORYVIEWPTR_H_

#include "MemoryView.h"

class CMemoryViewPtr : public CMemoryView
{
public:
						CMemoryViewPtr(HWND, RECT*);
	void				SetData(void*, uint32);

protected:
	virtual uint8		GetByte(uint32);
	void*				m_pData;
};

#endif
