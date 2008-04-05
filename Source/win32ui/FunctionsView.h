#ifndef _FUNCTIONSVIEW_H_
#define _FUNCTIONSVIEW_H_

#include <boost/signal.hpp>
#include "win32/MDIChild.h"
#include "win32/ListView.h"
#include "win32/Button.h"
#include "layout/VerticalLayout.h"
#include "../MIPS.h"
#include "../ELF.h"

class CFunctionsView : public Framework::Win32::CMDIChild
{
public:
									CFunctionsView(HWND, CMIPS*);
									~CFunctionsView();
	void							SetELF(CELF*);
	void							Refresh();

	boost::signal<void (uint32)>	m_OnFunctionDblClick;
	boost::signal<void (void)>		m_OnFunctionsStateChange;

protected:
	long							OnSize(unsigned int, unsigned int, unsigned int);
	long							OnCommand(unsigned short, unsigned short, HWND);
	long							OnNotify(WPARAM, NMHDR*);
	long							OnSysCommand(unsigned int, LPARAM);

private:
	void							CreateListColumns();
	void							ResizeListColumns();
	void							RefreshLayout();
	void							RefreshList();

	void							OnListDblClick();
	void							OnNewClick();
	void							OnRenameClick();
	void							OnDeleteClick();
	void							OnImportClick();

	Framework::Win32::CListView*	m_pList;
	Framework::Win32::CButton*		m_pNew;
	Framework::Win32::CButton*		m_pRename;
	Framework::Win32::CButton*		m_pDelete;
	Framework::Win32::CButton*		m_pImport;

	Framework::CVerticalLayout*		m_pLayout;
	CMIPS*							m_pCtx;
	CELF*							m_pELF;
};

#endif
