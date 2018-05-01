#pragma once

#include <minmax.h>
#include <GdiPlus.h>
#include <memory>
#include <map>
#include "win32/Window.h"
#include "VirtualPadItem.h"

class CVirtualPadWindow : public Framework::Win32::CWindow
{
public:
	CVirtualPadWindow();
	CVirtualPadWindow(HWND);
	virtual ~CVirtualPadWindow();

	CVirtualPadWindow& operator=(CVirtualPadWindow&&);

	void SetPadHandler(CPH_Generic*);

protected:
	long OnSize(unsigned int, unsigned int, unsigned int) override;
	long OnLeftButtonDown(int, int) override;
	long OnLeftButtonUp(int, int) override;
	long OnMouseMove(WPARAM, int, int) override;
	LRESULT OnMouseActivate(WPARAM, LPARAM) override;

private:
	typedef std::shared_ptr<CVirtualPadItem> ItemPtr;
	typedef std::shared_ptr<Gdiplus::Bitmap> BitmapPtr;
	typedef std::vector<ItemPtr> ItemArray;
	typedef std::map<std::string, BitmapPtr> BitmapMap;

	void Reset();
	void MoveFrom(CVirtualPadWindow&&);

	void RecreateItems(unsigned int, unsigned int);
	void UpdateSurface();

	static BitmapPtr LoadBitmapFromResource(int);

	ULONG_PTR m_gdiPlusToken = 0;
	ItemArray m_items;
	BitmapMap m_itemImages;
};
