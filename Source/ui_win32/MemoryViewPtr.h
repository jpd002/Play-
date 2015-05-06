#pragma once

#include "MemoryView.h"

class CMemoryViewPtr : public CMemoryView
{
public:
						CMemoryViewPtr(HWND, const RECT&);
	void				SetData(void*, uint32);

protected:
	virtual uint8		GetByte(uint32) override;

	void*				m_pData;
};
