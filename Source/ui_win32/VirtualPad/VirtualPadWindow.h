#pragma once

#include <memory>
#include "win32/Window.h"
#include "VirtualPadItem.h"

class CVirtualPadWindow : public Framework::Win32::CWindow
{
public:
	           CVirtualPadWindow();
	           CVirtualPadWindow(HWND);
	virtual    ~CVirtualPadWindow();

	CVirtualPadWindow&    operator =(CVirtualPadWindow&&);

protected:
	long    OnSize(unsigned int, unsigned int ,unsigned int) override;
	long    OnLeftButtonDown(int, int) override;
	long    OnLeftButtonUp(int, int) override;
	long    OnMouseMove(WPARAM, int, int) override;
	long    OnMouseActivate(WPARAM, LPARAM) override;

private:
	typedef std::shared_ptr<CVirtualPadItem> ItemPtr;
	typedef std::vector<ItemPtr> ItemArray;

	void    Reset();
	void    MoveFrom(CVirtualPadWindow&&);

	void    RecreateItems(unsigned int, unsigned int);
	void    UpdateSurface();

	ULONG_PTR    m_gdiPlusToken = 0;
	ItemArray    m_items;
};
