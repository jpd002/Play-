#ifndef _NICESTATUS_H_
#define _NICESTATUS_H_

#include "win32/CustomDrawn.h"
#include "List.h"

enum PANELTYPE
{
	PANELTYPE_TEXT,
	PANELTYPE_OWNERDRAWN,
};

struct STATUSPANEL
{
	unsigned int	nType;
	double			nWidth;
	xchar*			sCaption;
	void*			pDrawFunc;
};

class CNiceStatus : public Framework::CCustomDrawn
{
public:
									CNiceStatus(HWND, RECT*);
									~CNiceStatus();
	void							InsertPanel(unsigned long, double, const xchar*);
	void							SetCaption(unsigned long, const xchar*);
protected:
	void							Paint(HDC);

private:
	HFONT							CreateOurFont();
	Framework::CList<STATUSPANEL>	m_Panel;	
};

#endif