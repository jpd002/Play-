#include "RegViewWnd.h"
#include "RegViewGeneral.h"
#include "RegViewSCU.h"
#include "RegViewFPU.h"
#include "RegViewVU.h"
#include "PtrMacro.h"

#define CLSNAME		_T("CRegViewWnd")

using namespace Framework;
using namespace boost;

CRegViewWnd::CRegViewWnd(HWND hParent, CVirtualMachine& virtualMachine, CMIPS* pCtx)
{
	RECT rc;

	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW); 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		RegisterClassEx(&wc);
	}
	
	SetRect(&rc, 0, 0, 320, 240);

	Create(NULL, CLSNAME, _T("Registers"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, &rc, hParent, NULL);
	SetClassPtr();

	m_pRegView[0] = new CRegViewGeneral(m_hWnd, rc, virtualMachine, pCtx);
	m_pRegView[1] = new CRegViewSCU(m_hWnd, rc, virtualMachine, pCtx);
	m_pRegView[2] = new CRegViewFPU(m_hWnd, rc, virtualMachine, pCtx);
	m_pRegView[3] = new CRegViewVU(m_hWnd, rc, virtualMachine, pCtx);

	m_pRegView[0]->Enable(false);
	m_pRegView[1]->Enable(false);
	m_pRegView[2]->Enable(false);
	m_pRegView[3]->Enable(false);

	m_pRegView[0]->Show(SW_HIDE);
	m_pRegView[1]->Show(SW_HIDE);
	m_pRegView[2]->Show(SW_HIDE);
	m_pRegView[3]->Show(SW_HIDE);

	m_pTabs = new CNiceTabs(m_hWnd, rc);
	m_pTabs->InsertTab(_T("General"),	CNiceTabs::TAB_FLAG_UNDELETEABLE, 0);
	m_pTabs->InsertTab(_T("SCU"),		CNiceTabs::TAB_FLAG_UNDELETEABLE, 1);
	m_pTabs->InsertTab(_T("FPU"),		CNiceTabs::TAB_FLAG_UNDELETEABLE, 2);
	m_pTabs->InsertTab(_T("VU"),		CNiceTabs::TAB_FLAG_UNDELETEABLE, 3);

	m_pCurrent = NULL;
	SetCurrentView(0);

	m_pTabs->OnTabChange.connect(bind(&CRegViewWnd::OnTabChange, this, _1));

	RefreshLayout();
}

CRegViewWnd::~CRegViewWnd()
{
	for(unsigned int i = 0; i < MAXTABS; i++)
	{
		DELETEPTR(m_pRegView[i]);
	}

	DELETEPTR(m_pTabs);
}

void CRegViewWnd::RefreshLayout()
{
	RECT rc = GetClientRect();

	if(m_pCurrent != NULL)
	{
		m_pCurrent->SetSize(rc.right, rc.bottom - 21);
	}

	m_pTabs->SetPosition(0, rc.bottom - 21);
	m_pTabs->SetSize(rc.right, 21);

}

long CRegViewWnd::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	RefreshLayout();
	return TRUE;
}

long CRegViewWnd::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

void CRegViewWnd::OnTabChange(unsigned int nTabID)
{
	SetCurrentView(nTabID);
}

void CRegViewWnd::SetCurrentView(unsigned int nView)
{
	if(m_pCurrent != NULL)
	{
		m_pCurrent->Enable(false);
		m_pCurrent->Show(SW_HIDE);
	}

	m_pCurrent = m_pRegView[nView];

	if(m_pCurrent != NULL)
	{
		m_pCurrent->Enable(true);
		m_pCurrent->Show(SW_SHOW);
		m_pCurrent->SetFocus();
	}

	RefreshLayout();
}
