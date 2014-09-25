#include "RegViewWnd.h"
#include "RegViewGeneral.h"
#include "RegViewSCU.h"
#include "RegViewFPU.h"
#include "RegViewVU.h"
#include "win32/DefaultWndClass.h"

#define WND_STYLE (WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX)

CRegViewWnd::CRegViewWnd(HWND parentWnd, CVirtualMachine& virtualMachine, CMIPS* ctx)
{
	auto windowRect = Framework::Win32::CRect(0, 0, 320, 240);

	Create(NULL, Framework::Win32::CDefaultWndClass::GetName(), _T("Registers"), WND_STYLE, windowRect, parentWnd, NULL);
	SetClassPtr();

	m_regView[0] = new CRegViewGeneral(m_hWnd, windowRect, virtualMachine, ctx);
	m_regView[1] = new CRegViewSCU(m_hWnd, windowRect, virtualMachine, ctx);
	m_regView[2] = new CRegViewFPU(m_hWnd, windowRect, virtualMachine, ctx);
	m_regView[3] = new CRegViewVU(m_hWnd, windowRect, virtualMachine, ctx);

	m_regView[0]->Enable(false);
	m_regView[1]->Enable(false);
	m_regView[2]->Enable(false);
	m_regView[3]->Enable(false);

	m_regView[0]->Show(SW_HIDE);
	m_regView[1]->Show(SW_HIDE);
	m_regView[2]->Show(SW_HIDE);
	m_regView[3]->Show(SW_HIDE);

	m_tabs = new CNiceTabs(m_hWnd, windowRect);
	m_tabs->InsertTab(_T("General"),CNiceTabs::TAB_FLAG_UNDELETEABLE, 0);
	m_tabs->InsertTab(_T("SCU"),	CNiceTabs::TAB_FLAG_UNDELETEABLE, 1);
	m_tabs->InsertTab(_T("FPU"),	CNiceTabs::TAB_FLAG_UNDELETEABLE, 2);
	m_tabs->InsertTab(_T("VU"),		CNiceTabs::TAB_FLAG_UNDELETEABLE, 3);

	SetCurrentView(0);

	m_tabs->OnTabChange.connect(bind(&CRegViewWnd::OnTabChange, this, _1));

	RefreshLayout();
}

CRegViewWnd::~CRegViewWnd()
{
	for(unsigned int i = 0; i < MAXTABS; i++)
	{
		delete m_regView[i];
	}
	delete m_tabs;
}

void CRegViewWnd::RefreshLayout()
{
	RECT rc = GetClientRect();

	if(m_current != nullptr)
	{
		m_current->SetSize(rc.right, rc.bottom - 21);
	}

	m_tabs->SetPosition(0, rc.bottom - 21);
	m_tabs->SetSize(rc.right, 21);

}

long CRegViewWnd::OnSize(unsigned int, unsigned int, unsigned int)
{
	RefreshLayout();
	return TRUE;
}

long CRegViewWnd::OnSysCommand(unsigned int cmd, LPARAM)
{
	switch(cmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

void CRegViewWnd::OnTabChange(unsigned int tabIndex)
{
	SetCurrentView(tabIndex);
}

void CRegViewWnd::SetCurrentView(unsigned int viewIndex)
{
	if(m_current != nullptr)
	{
		m_current->Enable(false);
		m_current->Show(SW_HIDE);
	}

	m_current = m_regView[viewIndex];

	if(m_current != nullptr)
	{
		m_current->Enable(true);
		m_current->Show(SW_SHOW);
		m_current->SetFocus();
	}

	RefreshLayout();
}
