#include "DirectXControl.h"
#include "win32/Rect.h"
#include <shlwapi.h>
#include <vsstyle.h>
#include <vssym32.h>

#define CLSNAME							_T("DirectXControl")
#define WNDSTYLE						(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP)
#define WNDSTYLEEX						(0)

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

CDirectXControl::CDirectXControl(HWND parentWnd, uint32 wndStyle)
: m_deviceLost(false)
, m_isThemeActive(false)
, m_theme(NULL)
, m_mouseInside(false)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX w;
		memset(&w, 0, sizeof(WNDCLASSEX));
		w.cbSize		= sizeof(WNDCLASSEX);
		w.lpfnWndProc	= CWindow::WndProc;
		w.lpszClassName	= CLSNAME;
		w.hbrBackground = NULL;
		w.hInstance		= GetModuleHandle(NULL);
		w.hCursor		= LoadCursor(NULL, IDC_ARROW);
		RegisterClassEx(&w);
	}

	InitializeTheme();

	Create(WNDSTYLEEX | (m_isThemeActive ? 0 : WS_EX_CLIENTEDGE), CLSNAME, _T(""), WNDSTYLE | wndStyle, Framework::Win32::CRect(0, 0, 1, 1), parentWnd, this);

	Initialize();
}

CDirectXControl::~CDirectXControl()
{
	if(m_theme != NULL)
	{
		CloseThemeData(m_theme);
	}
}

D3DCOLOR CDirectXControl::ConvertSysColor(DWORD color)
{
	uint8 r = static_cast<uint8>(color >>  0);
	uint8 g = static_cast<uint8>(color >>  8);
	uint8 b = static_cast<uint8>(color >> 16);
	return D3DCOLOR_XRGB(r, g, b);
}

long CDirectXControl::OnEraseBkgnd()
{
	return TRUE;
}

long CDirectXControl::OnPaint()
{
	Refresh();
	return TRUE;
}

long CDirectXControl::OnSize(unsigned int, unsigned int, unsigned int)
{
	ResetDevice();
	return TRUE;
}

long CDirectXControl::OnThemeChanged()
{
	if(m_theme)
	{
		CloseThemeData(m_theme);
		m_theme = NULL;
	}

	InitializeTheme();
	LONG exStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);
	exStyle &= ~WS_EX_CLIENTEDGE;
	exStyle |= (m_isThemeActive ? 0 : WS_EX_CLIENTEDGE);
	SetWindowLong(m_hWnd, GWL_EXSTYLE, exStyle);

	return TRUE;
}

long CDirectXControl::OnNcCalcSize(WPARAM wParam, LPARAM lParam)
{
	DefWindowProc(m_hWnd, WM_NCCALCSIZE, wParam, lParam);

	if(m_isThemeActive)
	{
		RECT* clientRect(NULL);
		if(wParam == 0)
		{
			clientRect = reinterpret_cast<RECT*>(lParam);
		}
		else
		{
			NCCALCSIZE_PARAMS* calcSizeParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
			clientRect = &calcSizeParams->rgrc[0];
		}

		RECT srcRect, dstRect;
		CopyRect(&srcRect, clientRect);

		HDC hDC = GetWindowDC(m_hWnd);
		HRESULT result = GetThemeBackgroundContentRect(m_theme, hDC, EP_EDITTEXT, ETS_NORMAL, &srcRect, &dstRect);
		assert(SUCCEEDED(result));
		ReleaseDC(m_hWnd, hDC);

		InflateRect(&dstRect, -1, -1);

		SetRect(&m_borderRect, 
			dstRect.left - srcRect.left,
			dstRect.top - srcRect.top,
			srcRect.right - dstRect.right,
			srcRect.bottom - dstRect.bottom); 

		CopyRect(clientRect, &dstRect);
	}

	return FALSE;
}

long CDirectXControl::OnNcPaint(WPARAM wParam)
{
	DefWindowProc(m_hWnd, WM_NCPAINT, wParam, 0);

	if(m_isThemeActive)
	{
		HDC hDC = GetWindowDC(m_hWnd);

		RECT windowRect = GetWindowRect();
		windowRect.bottom -= windowRect.top;
		windowRect.right -= windowRect.left;
		windowRect.top = 0;
		windowRect.left = 0;

		RECT clientRect;
		CopyRect(&clientRect, &windowRect);
		clientRect.left		+= m_borderRect.left;
		clientRect.top		+= m_borderRect.top;
		clientRect.right	-= m_borderRect.right;
		clientRect.bottom	-= m_borderRect.bottom;

		ExcludeClipRect(hDC, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

		int partId = EPSHV_NORMAL;
		if(m_mouseInside)
		{
			partId = EPSHV_HOT;
		}
		if(GetFocus() == m_hWnd)
		{
			partId = EPSHV_FOCUSED;
		}

		HRESULT result = DrawThemeBackground(m_theme, hDC, EP_EDITBORDER_HVSCROLL, partId, &windowRect, NULL);
		assert(SUCCEEDED(result));
		ReleaseDC(m_hWnd, hDC);
	}

	return FALSE;
}

long CDirectXControl::OnMouseMove(WPARAM, int, int)
{
	bool mustRefresh = false;
	if(!m_mouseInside)
	{
		mustRefresh = true;
	}
	m_mouseInside = true;
	if(mustRefresh)
	{
		RedrawWindow(m_hWnd, NULL, 0, RDW_FRAME | RDW_INVALIDATE);
	}
	TRACKMOUSEEVENT trackMouse;
	memset(&trackMouse, 0, sizeof(TRACKMOUSEEVENT));
	trackMouse.cbSize		= sizeof(TRACKMOUSEEVENT);
	trackMouse.dwFlags		= TME_LEAVE;
	trackMouse.hwndTrack	= m_hWnd;
	TrackMouseEvent(&trackMouse);
	return TRUE;
}

long CDirectXControl::OnMouseLeave()
{
	m_mouseInside = false;
	RedrawWindow(m_hWnd, NULL, 0, RDW_FRAME | RDW_INVALIDATE);
	return TRUE;
}

long CDirectXControl::OnLeftButtonDown(int, int)
{
	SetFocus();
	RedrawWindow(m_hWnd, NULL, 0, RDW_FRAME | RDW_INVALIDATE);
	return TRUE;
}

long CDirectXControl::OnRightButtonDown(int, int)
{
	SetFocus();
	RedrawWindow(m_hWnd, NULL, 0, RDW_FRAME | RDW_INVALIDATE);
	return TRUE;
}

long CDirectXControl::OnSetFocus()
{
	RedrawWindow(m_hWnd, NULL, 0, RDW_FRAME | RDW_INVALIDATE);
	return TRUE;
}

long CDirectXControl::OnKillFocus()
{
	RedrawWindow(m_hWnd, NULL, 0, RDW_FRAME | RDW_INVALIDATE);
	return TRUE;
}

void CDirectXControl::Initialize()
{
	m_d3d = Framework::Win32::CComPtr<IDirect3D9>(Direct3DCreate9(D3D_SDK_VERSION));
	CreateDevice();
}

void CDirectXControl::InitializeTheme()
{
	typedef void (WINAPI *DllGetVersionProc)(DLLVERSIONINFO*);

	HMODULE comCtlModule = LoadLibrary(_T("comctl32.dll"));
	DllGetVersionProc comCtlGetVersion = reinterpret_cast<DllGetVersionProc>(GetProcAddress(comCtlModule, "DllGetVersion"));

	DLLVERSIONINFO comCtlVersion;
	memset(&comCtlVersion, 0, sizeof(DLLVERSIONINFO));
	comCtlVersion.cbSize = sizeof(DLLVERSIONINFO);
	comCtlGetVersion(&comCtlVersion);
	FreeLibrary(comCtlModule);

	BOOL themed = IsAppThemed();
	BOOL themeActive = IsThemeActive();

	m_isThemeActive = (comCtlVersion.dwMajorVersion > 5) && themed && themeActive;

	if(m_isThemeActive)
	{
		m_theme = OpenThemeData(m_hWnd, VSCLASS_EDIT);
	}

	SetRect(&m_borderRect, 0, 0, 1, 1);
}

void CDirectXControl::CreateDevice()
{
	D3DPRESENT_PARAMETERS d3dpp(CreatePresentParams());
	HRESULT result = m_d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		m_hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&m_device);

	if(FAILED(result))
	{
		return;
	}

	m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	m_device->SetRenderState(D3DRS_LIGHTING, FALSE);

	m_deviceLost = false;
}

void CDirectXControl::ResetDevice()
{
	if(m_device.IsEmpty()) return;
	OnDeviceResetting();
	D3DPRESENT_PARAMETERS d3dpp(CreatePresentParams());
	HRESULT result = m_device->Reset(&d3dpp);
	if(SUCCEEDED(result))
	{
		m_deviceLost = false;
		OnDeviceReset();
	}
	else
	{
		assert(0);
	}
}

bool CDirectXControl::TestDevice()
{
	HRESULT coopLevelResult = m_device->TestCooperativeLevel();
	if(FAILED(coopLevelResult))
	{
		if(coopLevelResult == D3DERR_DEVICELOST)
		{
			m_deviceLost = true;
		}
		else if(coopLevelResult == D3DERR_DEVICENOTRESET)
		{
			m_deviceLost = true;
			ResetDevice();
		}
		else
		{
			assert(0);
		}
	}

	return !m_deviceLost;
}

D3DPRESENT_PARAMETERS CDirectXControl::CreatePresentParams()
{
	RECT clientRect = GetClientRect();

	int clientAreaWidth = clientRect.right - clientRect.left;
	int clientAreaHeight = clientRect.bottom - clientRect.top;
	clientAreaWidth = std::max<int>(clientAreaWidth, 1);
	clientAreaHeight = std::max<int>(clientAreaHeight, 1);

	D3DPRESENT_PARAMETERS d3dpp;
	memset(&d3dpp, 0, sizeof(D3DPRESENT_PARAMETERS));
	d3dpp.Windowed					= TRUE;
	d3dpp.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow				= m_hWnd;
	d3dpp.BackBufferFormat			= D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth			= clientAreaWidth;
	d3dpp.BackBufferHeight			= clientAreaHeight;
	d3dpp.EnableAutoDepthStencil	= TRUE;
	d3dpp.AutoDepthStencilFormat	= D3DFMT_D16;
	return d3dpp;
}
