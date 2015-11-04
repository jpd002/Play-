#include <minmax.h>
#include <gdiplus.h>
#include "VirtualPadWindow.h"
#include "VirtualPadButton.h"
#include "../../VirtualPad.h"
#include "win32/ClientDeviceContext.h"
#include "win32/MemoryDeviceContext.h"
#include "win32/GdiObj.h"
#include "string_cast.h"

CVirtualPadWindow::CVirtualPadWindow()
{

}

CVirtualPadWindow::CVirtualPadWindow(HWND parentWnd)
{
	Gdiplus::GdiplusStartupInput startupInput = {};
	auto result = Gdiplus::GdiplusStartup(&m_gdiPlusToken, &startupInput, NULL);
	assert(result == Gdiplus::Ok);

	Create(WS_EX_TRANSPARENT | WS_EX_LAYERED, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WS_POPUP, Framework::Win32::CRect(0, 0, 128, 128), parentWnd, NULL);
	SetClassPtr();
}

CVirtualPadWindow::~CVirtualPadWindow()
{
	Reset();
}

CVirtualPadWindow& CVirtualPadWindow::operator =(CVirtualPadWindow&& rhs)
{
	Reset();
	MoveFrom(std::move(rhs));
	return (*this);
}

long CVirtualPadWindow::OnSize(unsigned int, unsigned int width, unsigned int height)
{
	RecreateItems(width, height);
	UpdateSurface();
	return TRUE;
}

long CVirtualPadWindow::OnLeftButtonDown(int x, int y)
{
	return TRUE;
}

void CVirtualPadWindow::Reset()
{
	if(m_gdiPlusToken != 0)
	{
		Gdiplus::GdiplusShutdown(m_gdiPlusToken);
		m_gdiPlusToken = 0;
	}
	m_items.clear();
}

void CVirtualPadWindow::MoveFrom(CVirtualPadWindow&& rhs)
{
	CWindow::MoveFrom(std::move(rhs));
	std::swap(m_gdiPlusToken, rhs.m_gdiPlusToken);
	m_items = std::move(rhs.m_items);
}

void CVirtualPadWindow::RecreateItems(unsigned int width, unsigned int height)
{
	auto itemDefs = CVirtualPad::GetItems(width, height);
	
	m_items.clear();
	for(const auto& itemDef : itemDefs)
	{
		Framework::Win32::CRect bounds(itemDef.x1, itemDef.y1, itemDef.x2, itemDef.y2);
		ItemPtr item;
		if(itemDef.isAnalog)
		{
			continue;
		}
		else
		{
			auto button = std::make_shared<CVirtualPadButton>();
			button->SetCaption(string_cast<std::wstring>(itemDef.caption));
			item = button;
		}
		item->SetBounds(bounds);
		m_items.push_back(item);
	}
}

void CVirtualPadWindow::UpdateSurface()
{
	auto windowRect = GetWindowRect();

	auto screenDc = Framework::Win32::CClientDeviceContext(NULL);
	auto memDc = Framework::Win32::CMemoryDeviceContext(screenDc);
	auto memBitmap = Framework::Win32::CBitmap(CreateCompatibleBitmap(screenDc, windowRect.Width(), windowRect.Height()));
	memDc.SelectObject(memBitmap);

	Gdiplus::Graphics graphics(memDc);
	for(const auto& item : m_items)
	{
		item->Draw(graphics);
	}

	POINT dstPt = { windowRect.Left(), windowRect.Top() };
	SIZE dstSize = { windowRect.Width(), windowRect.Height() };
	POINT srcPt = { 0, 0 };

	BLENDFUNCTION blendFunc = {};
	blendFunc.AlphaFormat = AC_SRC_ALPHA;

	BOOL result = UpdateLayeredWindow(m_hWnd, screenDc, &dstPt, &dstSize, memDc, &srcPt, RGB(0, 0, 0), nullptr, ULW_COLORKEY);
	assert(result == TRUE);
}
