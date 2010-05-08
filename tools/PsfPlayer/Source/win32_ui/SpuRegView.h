#ifndef _SPUREGVIEW_H_
#define _SPUREGVIEW_H_

#include "DirectXControl.h"
#include "iop/Iop_SpuBase.h"

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

	virtual void			Refresh();
	virtual void			OnDeviceLost();
	virtual void			OnDeviceReset();

private:
	class CLineDrawer
	{
	public:
							CLineDrawer(LPD3DXFONT, D3DCOLOR, int, int);
		void				Draw(const TCHAR*, int = -1);
		void				Feed();

	private:
		int					m_posX;
		int					m_posY;
		LPD3DXFONT			m_font;
		D3DCOLOR			m_color;
	};

	void					CreateResources();
	void					InitializeScrollBars();

	void					UpdateHorizontalScroll();
	void					UpdateVerticalScroll();

	Iop::CSpuBase*			m_spu;
	LPD3DXFONT				m_font;

	std::tstring			m_title;

	int						m_offsetX;
	int						m_offsetY;
	int						m_pageSizeX;
	int						m_pageSizeY;
	int						m_maxScrollX;
	int						m_maxScrollY;
};

#endif
