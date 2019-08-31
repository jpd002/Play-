#include "RegViewWnd.h"
#include "RegViewGeneral.h"
#include "RegViewSCU.h"
#include "RegViewFPU.h"
#include "RegViewVU.h"

//#define WND_STYLE (WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX)

CRegViewWnd::CRegViewWnd(QWidget* parent, CMIPS* ctx)
	: QTabWidget(parent)
{
	this->setTabPosition(QTabWidget::South);
	this->setMinimumHeight(700);
	this->setMinimumWidth(320);

	m_regView[0] = new CRegViewGeneral(this, ctx);
	m_regView[1] = new CRegViewSCU(this, ctx);
	m_regView[2] = new CRegViewFPU(this, ctx);
	m_regView[3] = new CRegViewVU(this, ctx);

	//m_tabs = Framework::Win32::CTab(m_hWnd, windowRect, TCS_BOTTOM);
	this->addTab(m_regView[0], "General");
	this->addTab(m_regView[1], "SCU");
	this->addTab(m_regView[2], "FPU");
	this->addTab(m_regView[3], "VU");

	for(unsigned int i = 0; i < MAXTABS; i++)
	{
		//m_regView[i]->Enable(false);
		//m_regView[i]->Show(SW_HIDE);
	}

	//SelectTab(0);

	//RefreshLayout();
}

CRegViewWnd::~CRegViewWnd()
{
	for(unsigned int i = 0; i < MAXTABS; i++)
	{
		if(m_regView[i] != nullptr)
			delete m_regView[i];
	}
}

void CRegViewWnd::HandleMachineStateChange()
{
	for(unsigned int i = 0; i < MAXTABS; i++)
	{
		//if(m_regView[i] != nullptr)
			m_regView[i]->Update();
	}
	//m_current->Update();
}
void CRegViewWnd::HandleRunningStateChange(CVirtualMachine::STATUS)
{
	for(unsigned int i = 0; i > MAXTABS; i++)
	{
		//if(m_regView[i] != nullptr)
			m_regView[i]->Update();
	}
}

void CRegViewWnd::RefreshLayout()
{
	//auto clientRect = GetClientRect();

	//m_tabs.SetSizePosition(clientRect);

	//if(m_current != nullptr)
	{
	//	auto displayAreaRect = m_tabs.GetDisplayAreaRect();
	//	displayAreaRect.ClientToScreen(m_tabs.m_hWnd);
	//	displayAreaRect.ScreenToClient(m_hWnd);
	//	m_current->SetSizePosition(displayAreaRect);
	//	SetWindowPos(m_tabs.m_hWnd, m_current->m_hWnd, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	}
}

//long CRegViewWnd::OnSize(unsigned int, unsigned int, unsigned int)
//{
	//RefreshLayout();
//	return TRUE;
//}

//long CRegViewWnd::OnSysCommand(unsigned int cmd, LPARAM)
//{
	//switch(cmd)
	//{
	//case SC_CLOSE:
		//Show(SW_HIDE);
		//return FALSE;
	//}
//	return TRUE;
//}

//LRESULT CRegViewWnd::OnNotify(WPARAM param, NMHDR* hdr)
//{
	//if(CWindow::IsNotifySource(&m_tabs, hdr))
	//{
	//	switch(hdr->code)
	//	{
	//	case TCN_SELCHANGING:
	//		UnselectTab(m_tabs.GetSelection());
	//		break;
	//	case TCN_SELCHANGE:
	//		SelectTab(m_tabs.GetSelection());
	//		break;
	//	}
	//}
//	return FALSE;
//}

void CRegViewWnd::SelectTab(unsigned int viewIndex)
{
	//assert(m_current == nullptr);

	//m_current = m_regView[viewIndex];

	//if(m_current != nullptr)
	//{
	//	m_current->Update();
	//	m_current->Enable(true);
	//	m_current->Show(SW_SHOW);
	//	m_current->SetFocus();
	//}

	RefreshLayout();
}

void CRegViewWnd::UnselectTab(unsigned int viewIndex)
{
	//if(m_current != nullptr)
	//{
	//	m_current->Enable(false);
	//	m_current->Show(SW_HIDE);
	//}

	//m_current = nullptr;
}
