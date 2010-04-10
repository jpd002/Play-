#include "DirectXControl.h"
#include "win32/Rect.h"

#define CLSNAME			                _T("DirectXControl")
#define WNDSTYLE		                (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)
#define WNDSTYLEEX		                (0)

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

CDirectXControl::CDirectXControl(HWND parentWnd)
: m_d3d(NULL)
, m_device(NULL)
, m_deviceLost(false)
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

	Create(WNDSTYLEEX, CLSNAME, _T(""), WNDSTYLE, Framework::Win32::CRect(0, 0, 1, 1), parentWnd, NULL);
	SetClassPtr();

	m_backgroundColor = ConvertSysColor(GetSysColor(COLOR_BTNFACE));
	m_textColor = ConvertSysColor(GetSysColor(COLOR_WINDOWTEXT));

	Initialize();
}

CDirectXControl::~CDirectXControl()
{
	if(m_device != NULL)
	{
		m_device->Release();
		m_device = NULL;
	}

	if(m_d3d != NULL)
	{
		m_d3d->Release();
		m_d3d = NULL;
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
	RecreateDevice();
	CreateResources();
	return TRUE;
}

void CDirectXControl::Initialize()
{
	m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
	RecreateDevice();
}

void CDirectXControl::RecreateDevice()
{
	if(m_device)
	{
		m_device->Release();
		m_device = NULL;
	}

    D3DPRESENT_PARAMETERS d3dpp(CreatePresentParams());
    m_d3d->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      m_hWnd,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &m_device);

	m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    m_device->SetRenderState(D3DRS_LIGHTING, FALSE);

	m_deviceLost = false;
}

bool CDirectXControl::TestDevice()
{
	HRESULT coopLevelResult = m_device->TestCooperativeLevel();
	if(coopLevelResult == D3DERR_DEVICELOST)
	{
		m_deviceLost = true;
	}
	if(coopLevelResult == D3DERR_DEVICENOTRESET)
	{
	    D3DPRESENT_PARAMETERS d3dpp(CreatePresentParams());
		if(SUCCEEDED(m_device->Reset(&d3dpp)))
		{
			CreateResources();
			m_deviceLost = false;
		}
	}

	return !m_deviceLost;
}

D3DPRESENT_PARAMETERS CDirectXControl::CreatePresentParams()
{
	RECT clientRect = GetClientRect();

	D3DPRESENT_PARAMETERS d3dpp;
	memset(&d3dpp, 0, sizeof(D3DPRESENT_PARAMETERS));
    d3dpp.Windowed					= TRUE;
    d3dpp.SwapEffect				= D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow				= m_hWnd;
    d3dpp.BackBufferFormat			= D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth			= clientRect.right;
    d3dpp.BackBufferHeight			= clientRect.bottom;
	d3dpp.EnableAutoDepthStencil	= TRUE;
	d3dpp.AutoDepthStencilFormat	= D3DFMT_D16;
	return d3dpp;
}
