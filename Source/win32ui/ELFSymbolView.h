#ifndef _ELFSYMBOLVIEW_H_
#define _ELFSYMBOLVIEW_H_

#include "win32/Window.h"
#include "win32/ListView.h"
#include "../ELF.h"

class CELFSymbolView : public Framework::CWindow
{
public:
									CELFSymbolView(HWND, CELF*);
									~CELFSymbolView();
private:
	Framework::Win32::CListView*	m_pListView;
	long							OnSize(unsigned int, unsigned int, unsigned int);
	void							RefreshLayout();
	void							PopulateList();
	CELF*							m_pELF;
};

#endif