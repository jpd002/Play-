#include <minmax.h>
#include <gdiplus.h>
#include "VirtualPadWindow.h"
#include "../VirtualPad.h"
#include "win32/ClientDeviceContext.h"
#include "win32/MemoryDeviceContext.h"
#include "win32/GdiObj.h"

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

long CVirtualPadWindow::OnSize(unsigned int, unsigned int, unsigned int)
{
	Update();
	return TRUE;
}

void CVirtualPadWindow::Reset()
{
	if(m_gdiPlusToken != 0)
	{
		Gdiplus::GdiplusShutdown(m_gdiPlusToken);
		m_gdiPlusToken = 0;
	}
}

void CVirtualPadWindow::MoveFrom(CVirtualPadWindow&& rhs)
{
	CWindow::MoveFrom(std::move(rhs));
	std::swap(m_gdiPlusToken, rhs.m_gdiPlusToken);
}

void CVirtualPadWindow::Update()
{
	auto windowRect = GetWindowRect();

	auto items = CVirtualPad::GetItems(windowRect.Width(), windowRect.Height());

	auto screenDc = Framework::Win32::CClientDeviceContext(NULL);
	auto memDc = Framework::Win32::CMemoryDeviceContext(screenDc);
	auto memBitmap = Framework::Win32::CBitmap(CreateCompatibleBitmap(screenDc, windowRect.Width(), windowRect.Height()));
	memDc.SelectObject(memBitmap);

	Gdiplus::Graphics graphics(memDc);
	Gdiplus::SolidBrush myBrush(Gdiplus::Color(255, 0, 0, 255));

	for(const auto& item : items)
	{
		graphics.FillRectangle(&myBrush, item.x1, item.y1, item.x2 - item.x1, item.y2 - item.y1);
	}

	POINT dstPt = { windowRect.Left(), windowRect.Top() };
	SIZE dstSize = { windowRect.Width(), windowRect.Height() };
	POINT srcPt = { 0, 0 };

	BLENDFUNCTION blendFunc = {};
	blendFunc.AlphaFormat = AC_SRC_ALPHA;

	BOOL result = UpdateLayeredWindow(m_hWnd, screenDc, &dstPt, &dstSize, memDc, &srcPt, RGB(0, 0, 0), &blendFunc, ULW_COLORKEY);
	assert(result == TRUE);
}
