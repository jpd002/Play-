#pragma once

#include "win32ui/DirectXControl.h"
#include "iop/Iop_SpuBase.h"
#include "win32/ComPtr.h"

class CSpuRegView : public CDirectXControl
{
public:
							CSpuRegView(HWND, const TCHAR*);
	virtual					~CSpuRegView();

	void					SetSpu(Iop::CSpuBase*);

protected:
	virtual long			OnSize(unsigned int, unsigned int, unsigned int);
	virtual long			OnTimer(WPARAM);

	virtual long			OnHScroll(unsigned int, unsigned int);
	virtual long			OnVScroll(unsigned int, unsigned int);
	virtual long			OnMouseWheel(short);
	virtual long			OnKeyDown(unsigned int);
	virtual long			OnGetDlgCode(WPARAM, LPARAM);

	virtual void			Refresh() override;
	virtual void			OnDeviceResetting() override;
	virtual void			OnDeviceReset() override;

private:
	typedef Framework::Win32::CComPtr<ID3DXFont> FontPtr;

	class CLineDrawer
	{
	public:
							CLineDrawer(const FontPtr&, D3DCOLOR, int, int);
		void				Draw(const TCHAR*, int = -1);
		void				Feed();

	private:
		int					m_posX;
		int					m_posY;
		FontPtr				m_font;
		D3DCOLOR			m_color;
	};

	void					CreateResources();
	void					InitializeScrollBars();

	void					UpdateHorizontalScroll();
	void					UpdateVerticalScroll();

	Iop::CSpuBase*			m_spu;
	FontPtr					m_font;

	std::tstring			m_title;

	int						m_offsetX;
	int						m_offsetY;
	int						m_pageSizeX;
	int						m_pageSizeY;
	int						m_maxScrollX;
	int						m_maxScrollY;
};
