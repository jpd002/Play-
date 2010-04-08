#ifndef _DIRECTXCONTROL_H_
#define _DIRECTXCONTROL_H_

#include "win32/Window.h"
#include <d3d9.h>
#include <d3dx9.h>

class CDirectXControl : public Framework::Win32::CWindow
{
public:
							CDirectXControl(HWND);
	virtual					~CDirectXControl();

protected:
	D3DCOLOR				ConvertSysColor(DWORD);

	void					Initialize();
	virtual void			RecreateDevice();

	virtual void			Refresh() = 0;

	virtual long			OnEraseBkgnd();
	virtual long			OnPaint();
	virtual long			OnSize(unsigned int, unsigned int, unsigned int);

	LPDIRECT3D9				m_d3d;
	LPDIRECT3DDEVICE9		m_device;

	D3DCOLOR				m_backgroundColor;
	D3DCOLOR				m_textColor;
};

#endif
