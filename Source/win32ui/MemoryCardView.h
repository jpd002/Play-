#ifndef _MEMORYCARDVIEW_H_
#define _MEMORYCARDVIEW_H_

#include "win32/Window.h"
#include "MemoryCard.h"
#include "IconView.h"
#include "Event.h"

class CMemoryCardView : public Framework::CWindow
{
public:
									CMemoryCardView(HWND, RECT*);
									~CMemoryCardView();

	long							OnDrawItem(unsigned int, LPDRAWITEMSTRUCT);
	void							SetMemoryCard(CMemoryCard*);

	Framework::CEvent<CSave*>		m_OnSelectionChange;

protected:
	long							OnPaint();
	long							OnTimer();
	long							OnVScroll(unsigned int, unsigned int);
	long							OnLeftButtonDown(int, int);
	long							OnMouseWheel(short);
	long							OnSize(unsigned int, unsigned int, unsigned int);
	long							OnKeyDown(unsigned int);

private:
	typedef boost::ptr_list<CIconView>	IconList;
	typedef std::list<CSave*>			SaveList;

	void							Animate();
	void							DrawScene();
	void							UpdateScroll();
	void							UpdateScrollPosition();
	int								GetCanvasSize();
	void							UpdateGeometry();
	void							SetSelection(unsigned int);
	void							EnsureItemFullyVisible(unsigned int);

	IconList						m_Icons;
	SaveList						m_Saves;

	double							m_nRotation;
	static PIXELFORMATDESCRIPTOR	m_PFD;
	HDC								m_hDC;
	HGLRC							m_hRC;
	
	unsigned int					m_nItemWidth;
	unsigned int					m_nItemHeight;
	unsigned int					m_nSelection;
	int								m_nScrollPosition;

	CMemoryCard*					m_pMemoryCard;
};

#endif
