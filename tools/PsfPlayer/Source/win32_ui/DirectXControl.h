#ifndef _DIRECTXCONTROL_H_
#define _DIRECTXCONTROL_H_

#include "win32/Window.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <uxtheme.h>

class CDirectXControl : public Framework::Win32::CWindow
{
public:
							CDirectXControl(HWND, uint32 = 0);
	virtual					~CDirectXControl();

protected:
	D3DCOLOR				ConvertSysColor(DWORD);

	bool					TestDevice();

	virtual void			Refresh() = 0;
	virtual void			OnDeviceLost() = 0;
	virtual void			OnDeviceReset() = 0;

	virtual long			OnEraseBkgnd();
	virtual long			OnPaint();
	virtual long			OnSize(unsigned int, unsigned int, unsigned int);
	virtual long			OnThemeChanged();
	virtual long			OnNcCalcSize(WPARAM, LPARAM);
	virtual long			OnNcPaint(WPARAM);
	virtual long			OnMouseMove(WPARAM, int, int);
	virtual long			OnMouseLeave();
	virtual long			OnLeftButtonDown(int, int);
	virtual long			OnRightButtonDown(int, int);
	virtual long			OnSetFocus();
	virtual long			OnKillFocus();

	LPDIRECT3DDEVICE9		m_device;

private:
	void					Initialize();
	void					InitializeTheme();
	void					CreateDevice();
	void					ResetDevice();
	D3DPRESENT_PARAMETERS	CreatePresentParams();

	LPDIRECT3D9				m_d3d;
	bool					m_deviceLost;
	bool					m_isThemeActive;

	HTHEME					m_theme;
	RECT					m_borderRect;
	bool					m_mouseInside;
};

#endif
