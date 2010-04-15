#ifndef _SPUREGVIEW_H_
#define _SPUREGVIEW_H_

#include "DirectXControl.h"
#include "iop/Iop_SpuBase.h"

class CSpuRegView : public CDirectXControl
{
public:
							CSpuRegView(HWND);
	virtual					~CSpuRegView();

	void					SetSpu(Iop::CSpuBase*);

protected:
	virtual long			OnTimer(WPARAM);

	virtual void			Refresh();
	virtual void			OnDeviceLost();
	virtual void			OnDeviceReset();

private:
	class CLineDrawer
	{
	public:
							CLineDrawer(LPD3DXFONT, D3DCOLOR);
		void				Draw(const TCHAR*, int = -1);
		void				Feed();

	private:
		int					m_posY;
		LPD3DXFONT			m_font;
		D3DCOLOR			m_color;
	};

	virtual void			CreateResources();

	Iop::CSpuBase*			m_spu;
	LPD3DXFONT				m_font;
};

#endif
