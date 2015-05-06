#include "RegViewWnd.h"
#include "RegViewGeneral.h"
#include "RegViewSCU.h"
#include "RegViewFPU.h"
#include "RegViewVU.h"
#include "win32/DefaultWndClass.h"

#define WND_STYLE (WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX)

CRegViewWnd::CRegViewWnd(HWND parentWnd, CVirtualMachine& virtualMachine, CMIPS* ctx)
{
	auto windowRect = Framework::Win32::CRect(0, 0, 320, 240);

	Create(NULL, Framework::Win32::CDefaultWndClass::GetName(), _T("Registers"), WND_STYLE, windowRect, parentWnd, NULL);
	SetClassPtr();

	m_tabs = Framework::Win32::CTab(m_hWnd, windowRect, TCS_BOTTOM);
	m_tabs.InsertTab(_T("General"));
	m_tabs.InsertTab(_T("SCU"));
	m_tabs.InsertTab(_T("FPU"));
	m_tabs.InsertTab(_T("VU"));

	m_regView[0] = new CRegViewGeneral(m_hWnd, windowRect, virtualMachine, ctx);
	m_regView[1] = new CRegViewSCU(m_hWnd, windowRect, virtualMachine, ctx);
	m_regView[2] = new CRegViewFPU(m_hWnd, windowRect, virtualMachine, ctx);
	m_regView[3] = new CRegViewVU(m_hWnd, windowRect, virtualMachine, ctx);

	for(unsigned int i = 0; i < MAXTABS; i++)
	{
		m_regView[i]->Enable(false);
		m_regView[i]->Show(SW_HIDE);
	}

	SelectTab(0);

	RefreshLayout();
}

CRegViewWnd::~CRegViewWnd()
{
	for(unsigned int i = 0; i < MAXTABS; i++)
	{
		delete m_regView[i];
	}
}

void CRegViewWnd::RefreshLayout()
{
	auto clientRect = GetClientRect();

	m_tabs.SetSizePosition(clientRect);

	if(m_current != nullptr)
	{
		auto displayAreaRect = m_tabs.GetDisplayAreaRect();
		displayAreaRect.ClientToScreen(m_tabs.m_hWnd);
		displayAreaRect.ScreenToClient(m_hWnd);
		m_current->SetSizePosition(displayAreaRect);
		SetWindowPos(m_tabs.m_hWnd, m_current->m_hWnd, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	}
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

long CRegViewWnd::OnNotify(WPARAM param, NMHDR* hdr)
{
	if(CWindow::IsNotifySource(&m_tabs, hdr))
	{
		switch(hdr->code)
		{
		case TCN_SELCHANGING:
			UnselectTab(m_tabs.GetSelection());
			break;
		case TCN_SELCHANGE:
			SelectTab(m_tabs.GetSelection());
			break;
		}
	}
	return FALSE;
}

void CRegViewWnd::SelectTab(unsigned int viewIndex)
{
	assert(m_current == nullptr);

	m_current = m_regView[viewIndex];

	if(m_current != nullptr)
	{
		m_current->Enable(true);
		m_current->Show(SW_SHOW);
		m_current->SetFocus();
	}

	RefreshLayout();
}

void CRegViewWnd::UnselectTab(unsigned int viewIndex)
{
	if(m_current != nullptr)
	{
		m_current->Enable(false);
		m_current->Show(SW_HIDE);
	}

	m_current = nullptr;
}
