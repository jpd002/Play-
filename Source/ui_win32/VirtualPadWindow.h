#pragma once

#include "win32/Window.h"

class CVirtualPadWindow : public Framework::Win32::CWindow
{
public:
	           CVirtualPadWindow();
	           CVirtualPadWindow(HWND);
	virtual    ~CVirtualPadWindow();

	CVirtualPadWindow&    operator =(CVirtualPadWindow&&);

protected:
	long    OnSize(unsigned int, unsigned int ,unsigned int) override;

private:
	void    Reset();
	void    MoveFrom(CVirtualPadWindow&&);

	void    Update();

	ULONG_PTR    m_gdiPlusToken = 0;
};
