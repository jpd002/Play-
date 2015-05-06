#include "string_format.h"
#include "win32/DpiUtils.h"
#include "MemoryViewMIPSWnd.h"

#define WNDSTYLE (WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX)

CMemoryViewMIPSWnd::CMemoryViewMIPSWnd(HWND parentWnd, CVirtualMachine& virtualMachine, CMIPS* ctx)
{
	auto wndRect = Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, 320, 240));

	Create(NULL, Framework::Win32::CDefaultWndClass::GetName(), _T("Memory"), WNDSTYLE, wndRect, parentWnd, NULL);
	SetClassPtr();

	m_addressEdit = new Framework::Win32::CEdit(m_hWnd, wndRect, _T(""), ES_READONLY);

	m_memoryView = new CMemoryViewMIPS(m_hWnd, wndRect, virtualMachine, ctx);
	m_memoryView->OnSelectionChange.connect(boost::bind(&CMemoryViewMIPSWnd::OnMemoryViewSelectionChange, this, _1));

	UpdateStatusBar();
	RefreshLayout();
}

CMemoryViewMIPSWnd::~CMemoryViewMIPSWnd()
{
	delete m_addressEdit;
	delete m_memoryView;
}

CMemoryViewMIPS* CMemoryViewMIPSWnd::GetMemoryView() const
{
	return m_memoryView;
}

long CMemoryViewMIPSWnd::OnSize(unsigned int nType, unsigned int nCX, unsigned int nCY)
{
	RefreshLayout();
	return TRUE;
}

long CMemoryViewMIPSWnd::OnSetFocus()
{
	m_memoryView->SetFocus();
	return FALSE;
}

long CMemoryViewMIPSWnd::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

void CMemoryViewMIPSWnd::UpdateStatusBar()
{
	auto caption = string_format(_T("Address : 0x%0.8X"), m_memoryView->GetSelection());
	m_addressEdit->SetText(caption.c_str());
}

void CMemoryViewMIPSWnd::RefreshLayout()
{
	auto rc = GetClientRect();

	const int addressEditHeight = Framework::Win32::PointsToPixels(21);

	m_addressEdit->SetSize(rc.Right(), addressEditHeight);
	m_memoryView->SetPosition(0, addressEditHeight);
	m_memoryView->SetSize(rc.Right(), rc.Bottom() - addressEditHeight);
}

void CMemoryViewMIPSWnd::OnMemoryViewSelectionChange(uint32 address)
{
	UpdateStatusBar();
}
