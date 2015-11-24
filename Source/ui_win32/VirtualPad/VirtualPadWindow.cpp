#include "VirtualPadWindow.h"
#include "VirtualPadButton.h"
#include "VirtualPadStick.h"
#include "../resource.h"
#include "../../VirtualPad.h"
#include "win32/ClientDeviceContext.h"
#include "win32/MemoryDeviceContext.h"
#include "win32/GdiObj.h"
#include "win32/ComPtr.h"
#include "string_cast.h"

#define POINTER_TOKEN 0xDEADBEEF

CVirtualPadWindow::CVirtualPadWindow()
{

}

CVirtualPadWindow::CVirtualPadWindow(HWND parentWnd)
{
	Gdiplus::GdiplusStartupInput startupInput = {};
	auto result = Gdiplus::GdiplusStartup(&m_gdiPlusToken, &startupInput, NULL);
	assert(result == Gdiplus::Ok);

	m_itemImages["left"]        = LoadBitmapFromResource(IDB_VIRTUALPAD_LEFT);
	m_itemImages["right"]       = LoadBitmapFromResource(IDB_VIRTUALPAD_RIGHT);
	m_itemImages["up"]          = LoadBitmapFromResource(IDB_VIRTUALPAD_UP);
	m_itemImages["down"]        = LoadBitmapFromResource(IDB_VIRTUALPAD_DOWN);
	m_itemImages["triangle"]    = LoadBitmapFromResource(IDB_VIRTUALPAD_TRIANGLE);
	m_itemImages["square"]      = LoadBitmapFromResource(IDB_VIRTUALPAD_SQUARE);
	m_itemImages["circle"]      = LoadBitmapFromResource(IDB_VIRTUALPAD_CIRCLE);
	m_itemImages["cross"]       = LoadBitmapFromResource(IDB_VIRTUALPAD_CROSS);
	m_itemImages["start"]       = LoadBitmapFromResource(IDB_VIRTUALPAD_START);
	m_itemImages["select"]      = LoadBitmapFromResource(IDB_VIRTUALPAD_SELECT);
	m_itemImages["lr"]          = LoadBitmapFromResource(IDB_VIRTUALPAD_LR);
	m_itemImages["analogStick"] = LoadBitmapFromResource(IDB_VIRTUALPAD_ANALOGSTICK);

	Create(WS_EX_LAYERED, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WS_POPUP, Framework::Win32::CRect(0, 0, 128, 128), parentWnd, NULL);
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

void CVirtualPadWindow::SetPadHandler(CPH_Generic* padHandler)
{
	for(auto& item : m_items)
	{
		item->SetPadHandler(padHandler);
	}
}

long CVirtualPadWindow::OnSize(unsigned int, unsigned int width, unsigned int height)
{
	RecreateItems(width, height);
	UpdateSurface();
	return TRUE;
}

long CVirtualPadWindow::OnLeftButtonDown(int x, int y)
{
	for(auto& item : m_items)
	{
		if(item->GetBounds().Contains(x, y))
		{
			SetCapture(m_hWnd);
			item->SetPointerId(POINTER_TOKEN);
			item->OnMouseDown(x, y);
			UpdateSurface();
			break;
		}
	}
	return TRUE;
}

long CVirtualPadWindow::OnLeftButtonUp(int x, int y)
{
	for(auto& item : m_items)
	{
		if(item->GetPointerId() == POINTER_TOKEN)
		{
			ReleaseCapture();
			item->SetPointerId(0);
			item->OnMouseUp();
			UpdateSurface();
			break;
		}
	}
	return TRUE;
}

long CVirtualPadWindow::OnMouseMove(WPARAM, int x, int y)
{
	for(auto& item : m_items)
	{
		if(item->GetPointerId() == POINTER_TOKEN)
		{
			item->OnMouseMove(x, y);
			UpdateSurface();
			break;
		}
	}
	return TRUE;
}

long CVirtualPadWindow::OnMouseActivate(WPARAM wParam, LPARAM lParam)
{
	return MA_NOACTIVATE;
}

void CVirtualPadWindow::Reset()
{
	m_items.clear();
	m_itemImages.clear();
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
	m_items = std::move(rhs.m_items);
	m_itemImages = std::move(rhs.m_itemImages);
}

CVirtualPadWindow::BitmapPtr CVirtualPadWindow::LoadBitmapFromResource(int resourceId)
{
	HRSRC resourceInfo = FindResource(GetModuleHandle(nullptr), MAKEINTRESOURCE(resourceId), _T("PNG"));
	assert(resourceInfo != nullptr);
	HGLOBAL resourceHandle = LoadResource(GetModuleHandle(nullptr), resourceInfo);
	HGLOBAL resourceCopyHandle = GlobalAlloc(GMEM_MOVEABLE, SizeofResource(GetModuleHandle(nullptr), resourceInfo));
	assert(resourceCopyHandle != NULL);
	auto resourceCopyPtr = GlobalLock(resourceCopyHandle);
	auto resourcePtr = LockResource(resourceHandle);
	memcpy(resourceCopyPtr, resourcePtr, SizeofResource(GetModuleHandle(nullptr), resourceInfo));
	GlobalUnlock(resourceCopyHandle);
	UnlockResource(resourceHandle);
	FreeResource(resourceHandle);

	BitmapPtr bitmap;

	{
		Framework::Win32::CComPtr<IStream> stream;
		HRESULT result = CreateStreamOnHGlobal(resourceCopyHandle, FALSE, &stream);
		assert(SUCCEEDED(result));
		if(SUCCEEDED(result))
		{
			bitmap = BitmapPtr(Gdiplus::Bitmap::FromStream(stream));
		}
	}

	GlobalFree(resourceCopyHandle);

	return bitmap;
}

void CVirtualPadWindow::RecreateItems(unsigned int width, unsigned int height)
{
	auto itemDefs = CVirtualPad::GetItems(width, height);
	
	m_items.clear();
	for(const auto& itemDef : itemDefs)
	{
		Gdiplus::RectF bounds(itemDef.x1, itemDef.y1, itemDef.x2 - itemDef.x1, itemDef.y2 - itemDef.y1);
		ItemPtr item;
		if(itemDef.isAnalog)
		{
			auto stick = std::make_shared<CVirtualPadStick>();
			stick->SetCodeX(itemDef.code0);
			stick->SetCodeY(itemDef.code1);
			item = stick;
		}
		else
		{
			auto button = std::make_shared<CVirtualPadButton>();
			button->SetCaption(string_cast<std::wstring>(itemDef.caption));
			button->SetCode(itemDef.code0);
			item = button;
		}
		auto imageIterator = m_itemImages.find(itemDef.imageName);
		if(imageIterator != std::end(m_itemImages))
		{
			item->SetImage(imageIterator->second.get());
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

	BITMAPINFO bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth       = windowRect.Width();
	bitmapInfo.bmiHeader.biHeight      = windowRect.Height();
	bitmapInfo.bmiHeader.biPlanes      = 1;
	bitmapInfo.bmiHeader.biBitCount    = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	
	auto memBitmap = Framework::Win32::CBitmap(CreateDIBSection(memDc, &bitmapInfo, DIB_RGB_COLORS, nullptr, NULL, 0));
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
	blendFunc.BlendOp = AC_SRC_OVER;
	blendFunc.AlphaFormat = AC_SRC_ALPHA;
	blendFunc.SourceConstantAlpha = 255;

	BOOL result = UpdateLayeredWindow(m_hWnd, screenDc, &dstPt, &dstSize, memDc, &srcPt, RGB(0, 0, 0), &blendFunc, ULW_ALPHA);
	assert(result == TRUE);
}
