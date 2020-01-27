#include "PixelBufferView.h"
#include <assert.h>
#include <vector>
#include <algorithm>
#include "../../resource.h"
#include "win32/FileDialog.h"
#include "bitmap/BMP.h"
#include "StdStreamUtils.h"
#include "string_format.h"

#define CLSNAME _T("PixelView")
#define WNDSTYLE (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP)
#define WNDSTYLEEX (0)

CPixelBufferView::CPixelBufferView(HWND parentWnd, const RECT& rect)
    // : Framework::Win32::CWindow(parent)
    : m_zoomFactor(1)
    , m_panX(0)
    , m_panY(0)
    , m_dragging(false)
    , m_dragBaseX(0)
    , m_dragBaseY(0)
    , m_panXDragBase(0)
    , m_panYDragBase(0)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = CLSNAME;
		wc.lpfnWndProc = CWindow::WndProc;
		wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		RegisterClassEx(&wc);
	}
	Create(WNDSTYLEEX | (WS_EX_CLIENTEDGE), CLSNAME, _T(""), WNDSTYLE | 0, Framework::Win32::CRect(0, 0, 1, 1), parentWnd, this);
	SetClassPtr();

	m_gs = std::make_unique<CGSH_OpenGLFramedebugger>(this);
	m_overlay = std::make_unique<CPixelBufferViewOverlay>(m_hWnd);
	SetSizePosition(rect);

	Refresh();
}

long CPixelBufferView::OnEraseBkgnd()
{
	return TRUE;
}

long CPixelBufferView::OnPaint()
{
	if(!m_init)
	{
		m_init = true;
	}
	Refresh();
	return TRUE;
}

void CPixelBufferView::SetPixelBuffers(PixelBufferArray pixelBuffers)
{
	m_pixelBuffers = std::move(pixelBuffers);
	//Update buffer titles
	{
		CPixelBufferViewOverlay::StringList titles;
		for(const auto& pixelBuffer : m_pixelBuffers)
		{
			titles.push_back(pixelBuffer.first);
		}
		//Save previously selected index
		int selectedIndex = m_overlay->GetSelectedPixelBufferIndex();
		auto titleCount = titles.size();
		m_overlay->SetPixelBufferTitles(std::move(titles));
		//Restore selected index
		if((selectedIndex != -1) && (selectedIndex < titleCount))
		{
			m_overlay->SetSelectedPixelBufferIndex(selectedIndex);
		}
	}
	CreateSelectedPixelBufferTexture();
	Refresh();
}

void CPixelBufferView::Refresh()
{
	if(!m_init)
		return;

	m_gs->Begin();
	{
		DrawCheckerboard();
		DrawPixelBuffer();
	}
	m_gs->PresentBackbuffer();
}

void CPixelBufferView::DrawCheckerboard()
{
	RECT clientRect = GetClientRect();
	float screenSizeVector[2] =
	    {
	        static_cast<float>(clientRect.right),
	        static_cast<float>(clientRect.bottom)};
	m_gs->DrawCheckerboard(screenSizeVector);
}

void CPixelBufferView::DrawPixelBuffer()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;
	const auto& pixelBufferBitmap = pixelBuffer->second;

	RECT clientRect = GetClientRect();

	float screenSizeVector[2] =
	    {
	        static_cast<float>(clientRect.right),
	        static_cast<float>(clientRect.bottom)};

	float bufferSizeVector[2] =
	    {
	        static_cast<float>(pixelBufferBitmap.GetWidth()),
	        static_cast<float>(pixelBufferBitmap.GetHeight())};

	m_gs->DrawPixelBuffer(screenSizeVector, bufferSizeVector, m_panX, m_panY, m_zoomFactor);
}

long CPixelBufferView::OnCommand(unsigned short id, unsigned short cmd, HWND hwndFrom)
{
	if(CWindow::IsCommandSource(m_overlay.get(), hwndFrom))
	{
		switch(cmd)
		{
		case CPixelBufferViewOverlay::COMMAND_SAVE:
			OnSaveBitmap();
			break;
		case CPixelBufferViewOverlay::COMMAND_FIT:
			FitBitmap();
			break;
		case CPixelBufferViewOverlay::COMMAND_PIXELBUFFER_CHANGED:
			CreateSelectedPixelBufferTexture();
			Refresh();
			break;
		}
	}
	return TRUE;
}

long CPixelBufferView::OnSize(unsigned int type, unsigned int x, unsigned int y)
{
	long result = Framework::Win32::CWindow::OnSize(type, x, y);
	Refresh();
	return result;
}

long CPixelBufferView::OnLeftButtonDown(int x, int y)
{
	SetCapture(m_hWnd);
	m_dragBaseX = x;
	m_dragBaseY = y;
	m_panXDragBase = m_panX;
	m_panYDragBase = m_panY;
	m_dragging = true;
	return Framework::Win32::CWindow::OnLeftButtonDown(x, y);
}

long CPixelBufferView::OnLeftButtonUp(int x, int y)
{
	m_dragging = false;
	ReleaseCapture();
	return Framework::Win32::CWindow::OnLeftButtonUp(x, y);
}

long CPixelBufferView::OnMouseMove(WPARAM wparam, int x, int y)
{
	if(m_dragging)
	{
		RECT clientRect = GetClientRect();
		m_panX = m_panXDragBase + (static_cast<float>(x - m_dragBaseX) / static_cast<float>(clientRect.right / 2)) / m_zoomFactor;
		m_panY = m_panYDragBase - (static_cast<float>(y - m_dragBaseY) / static_cast<float>(clientRect.bottom / 2)) / m_zoomFactor;
		Refresh();
	}
	return Framework::Win32::CWindow::OnMouseMove(wparam, x, y);
}

long CPixelBufferView::OnMouseWheel(int x, int y, short z)
{
	float newZoom = 0;
	z /= WHEEL_DELTA;
	if(z <= -1)
	{
		newZoom = m_zoomFactor / 2;
	}
	else if(z >= 1)
	{
		newZoom = m_zoomFactor * 2;
	}

	if(newZoom != 0)
	{
		auto clientRect = GetClientRect();
		POINT mousePoint = {x, y};
		ScreenToClient(m_hWnd, &mousePoint);
		float relPosX = static_cast<float>(mousePoint.x) / static_cast<float>(clientRect.Right());
		float relPosY = static_cast<float>(mousePoint.y) / static_cast<float>(clientRect.Bottom());
		relPosX = std::max(relPosX, 0.f);
		relPosX = std::min(relPosX, 1.f);
		relPosY = std::max(relPosY, 0.f);
		relPosY = std::min(relPosY, 1.f);

		relPosX = (relPosX - 0.5f) * 2;
		relPosY = (relPosY - 0.5f) * 2;

		float panModX = (1 - newZoom / m_zoomFactor) * relPosX;
		float panModY = (1 - newZoom / m_zoomFactor) * relPosY;
		m_panX += panModX;
		m_panY -= panModY;

		m_zoomFactor = newZoom;
		Refresh();
	}

	return TRUE;
}

const CPixelBufferView::PixelBuffer* CPixelBufferView::GetSelectedPixelBuffer()
{
	if(m_pixelBuffers.empty()) return nullptr;

	int selectedPixelBufferIndex = m_overlay->GetSelectedPixelBufferIndex();
	if(selectedPixelBufferIndex < 0) return nullptr;

	assert(selectedPixelBufferIndex < m_pixelBuffers.size());
	if(selectedPixelBufferIndex >= m_pixelBuffers.size()) return nullptr;

	return &m_pixelBuffers[selectedPixelBufferIndex];
}

void CPixelBufferView::CreateSelectedPixelBufferTexture()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;

	m_gs->LoadTextureFromBitmap(pixelBuffer->second);
}

void CPixelBufferView::OnSaveBitmap()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;
	auto pixelBufferBitmap = pixelBuffer->second;

	auto bpp = pixelBufferBitmap.GetBitsPerPixel();
	if((bpp == 24) || (bpp == 32))
	{
		pixelBufferBitmap = ConvertBGRToRGB(std::move(pixelBufferBitmap));
	}

	Framework::Win32::CFileDialog openFileDialog;
	openFileDialog.m_OFN.lpstrFilter = _T("Windows Bitmap Files (*.bmp)\0*.bmp");
	int result = openFileDialog.SummonSave(m_hWnd);
	if(result == IDOK)
	{
		try
		{
			auto outputStream = Framework::CreateOutputStdStream(std::tstring(openFileDialog.m_OFN.lpstrFile));
			Framework::CBMP::WriteBitmap(pixelBufferBitmap, outputStream);
		}
		catch(const std::exception& exception)
		{
			auto message = string_format("Failed to save buffer to file:\r\n\r\n%s", exception.what());
			MessageBoxA(m_hWnd, message.c_str(), nullptr, MB_ICONHAND);
		}
	}
}

Framework::CBitmap CPixelBufferView::ConvertBGRToRGB(Framework::CBitmap bitmap)
{
	for(int32 y = 0; y < bitmap.GetHeight(); y++)
	{
		for(int32 x = 0; x < bitmap.GetWidth(); x++)
		{
			auto color = bitmap.GetPixel(x, y);
			std::swap(color.r, color.b);
			bitmap.SetPixel(x, y, color);
		}
	}
	return std::move(bitmap);
}

void CPixelBufferView::FitBitmap()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;
	const auto& pixelBufferBitmap = pixelBuffer->second;

	Framework::Win32::CRect clientRect = GetClientRect();
	unsigned int marginSize = 50;
	float normalizedSizeX = static_cast<float>(pixelBufferBitmap.GetWidth()) / static_cast<float>(clientRect.Right() - marginSize);
	float normalizedSizeY = static_cast<float>(pixelBufferBitmap.GetHeight()) / static_cast<float>(clientRect.Bottom() - marginSize);

	float normalizedSize = std::max<float>(normalizedSizeX, normalizedSizeY);

	m_zoomFactor = 1 / normalizedSize;
	m_panX = 0;
	m_panY = 0;

	Refresh();
}
