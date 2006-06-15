#ifndef _RENDERERSETTINGSWND_H_
#define _RENDERERSETTINGSWND_H_

#include "ModalWindow.h"
#include "VerticalLayout.h"
#include "win32/ListView.h"
#include "win32/Button.h"
#include "GSH_OpenGL.h"

class CRendererSettingsWnd : public CModalWindow
{
public:
								CRendererSettingsWnd(HWND, CGSH_OpenGL*);
	virtual						~CRendererSettingsWnd();

protected:
	long						OnCommand(unsigned short, unsigned short, HWND);

private:
	void						RefreshLayout();
	void						CreateExtListColumns();
	void						UpdateExtList();
	void						Save();

	Framework::CVerticalLayout*	m_pLayout;
	Framework::CListView*		m_pExtList;
	Framework::CButton*			m_pLineCheck;
	Framework::CButton*			m_pForceBilinearCheck;
	Framework::CButton*			m_pOk;
	Framework::CButton*			m_pCancel;
	CGSH_OpenGL*				m_pRenderer;

	bool						m_nLinesAsQuads;
	bool						m_nForceBilinearTextures;
};

#endif
