#ifndef _OPTIONWINDOW_H_
#define _OPTIONWINDOW_H_

#include "win32/Window.h"
#include "win32/TreeView.h"
#include "win32/Static.h"
#include "HorizontalLayout.h"

template <typename T> class COptionWnd : public T
{
public:
									COptionWnd(HWND, xchar*);
									~COptionWnd();

protected:
	long							OnNotify(WPARAM, NMHDR*);
	long							OnSize(unsigned int, unsigned int, unsigned int);

	Framework::CWindow*				GetContainer();
	Framework::CTreeView*			GetTreeView();
	HTREEITEM						InsertOption(HTREEITEM, const xchar*, HWND);
	void							DeleteAllOptions();
	void							RefreshLayout();
	void							UpdatePanel(TVITEM*, TVITEM*);

private:
	void							ResizePanel(HWND);
	Framework::CTreeView*			m_pTreeView;
	Framework::Win32::CStatic*		m_pContainer;
	Framework::CHorizontalLayout*	m_pLayout;


};

#endif
