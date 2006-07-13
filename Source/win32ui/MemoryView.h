#ifndef _MEMORYVIEW_H_
#define _MEMORYVIEW_H_

#include "Types.h"
#include "win32/CustomDrawn.h"

class CMemoryView : public Framework::CCustomDrawn
{
public:
					CMemoryView(HWND, RECT*);
	virtual			~CMemoryView();
	void			SetMemorySize(uint32);
	void			ScrollToAddress(uint32);

protected:
	virtual uint8	GetByte(uint32) = 0;
	virtual HFONT	GetFont();

	void			Paint(HDC);
	long			OnSize(unsigned int, unsigned int, unsigned int);
	long			OnVScroll(unsigned int, unsigned int);
	long			OnSetFocus();
	long			OnMouseWheel(short);
	long			OnLeftButtonDown(int, int);

private:
	void			GetVisibleRowsCols(unsigned int*, unsigned int*);
	void			UpdateScrollRange();
	unsigned int	GetScrollOffset();
	unsigned int	GetScrollThumbPosition();
	uint32			m_nSize;
	unsigned int	m_nPos;
};

#endif